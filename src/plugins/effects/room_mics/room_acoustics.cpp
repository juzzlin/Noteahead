// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "room_acoustics.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead::RoomAcoustics {

namespace {

//! ln(10^6): sixty decibels of energy, in nepers.
constexpr double sixtyDbNepers { 13.815510557964274 };

//! How close to a wall anything may stand.
constexpr double wallMarginM { 0.05 };
//! How close to the ceiling a source or a mic may hang.
constexpr double ceilingMarginM { 0.1 };

//! The most bounces off one kind of surface the reflection tables cover. Far more than a window
//! reaches in the smallest room the dimensions allow.
constexpr size_t maxBounces { 256 };

//! The crossovers of the band filter bank, halfway between the bands on a log scale.
constexpr std::array<double, BandCount - 1> crossoverHz { 176.8, 353.6, 707.1, 1414.2, 2828.4, 5656.9 };

//! What one step of each stage costs in work units, an image deposited into one response being
//! one. Measured, roughly: what keeps the job evenly spread over the audio.
constexpr double enumerateCandidateWork { 0.05 };
constexpr double filterSampleWork { 0.25 };

double length2d(double x, double y)
{
    return std::sqrt(x * x + y * y);
}

double clampInside(double value, double size)
{
    return std::clamp(value, std::min(wallMarginM, size * 0.5), std::max(size - wallMarginM, size * 0.5));
}

//! A unit vector level with the floor pointing from @p from to @p to, or towards the front wall
//! when the two are in the same place.
Vec3 aimFrom(const Vec3 & from, const Vec3 & to)
{
    const double dx = to.x - from.x;
    const double dy = to.y - from.y;
    if (const double length = length2d(dx, dy); length > 1.0e-3) {
        return { dx / length, dy / length, 0.0 };
    }
    return { 0.0, -1.0, 0.0 };
}

} // namespace

const Bands & absorption(Material material)
{
    // 125 Hz to 4 kHz from the common tables, 8 kHz taken as 4 kHz
    static const std::array<Bands, MaterialCount> table { {
      { 0.01, 0.01, 0.01, 0.01, 0.02, 0.02, 0.02 }, // Stone: marble, glazed tile
      { 0.01, 0.01, 0.02, 0.02, 0.02, 0.03, 0.03 }, // Concrete, poured and sealed
      { 0.03, 0.03, 0.03, 0.04, 0.05, 0.07, 0.07 }, // Brick, unglazed
      { 0.01, 0.02, 0.02, 0.03, 0.04, 0.05, 0.05 }, // Plaster on masonry
      { 0.15, 0.11, 0.10, 0.07, 0.06, 0.07, 0.07 }, // Wood: boards on joists
      { 0.35, 0.25, 0.18, 0.12, 0.07, 0.04, 0.04 }, // Glass: ordinary window
      { 0.02, 0.06, 0.14, 0.37, 0.60, 0.65, 0.65 }, // Carpet on concrete
      { 0.14, 0.35, 0.55, 0.72, 0.70, 0.65, 0.65 }, // Curtain: heavy velour, draped
      { 0.50, 0.70, 0.60, 0.70, 0.70, 0.50, 0.50 }, // Acoustic panel
      { 0.39, 0.57, 0.80, 0.94, 0.92, 0.87, 0.87 }, // Audience on upholstered seats
    } };
    return table.at(std::min(static_cast<size_t>(material), MaterialCount - 1));
}

const Bands & airAbsorptionDbPerMetre()
{
    static const Bands table { 0.0003, 0.0011, 0.0028, 0.0050, 0.0090, 0.0229, 0.0766 };
    return table;
}

double patternGain(Pattern pattern, double cosine)
{
    switch (pattern) {
    case Pattern::Omni:
        return 1.0;
    case Pattern::Cardioid:
        return 0.5 * (1.0 + cosine);
    case Pattern::FigureEight:
        return cosine;
    }
    return 1.0;
}

double randomEnergyEfficiency(Pattern pattern)
{
    switch (pattern) {
    case Pattern::Omni:
        return 1.0;
    case Pattern::Cardioid:
    case Pattern::FigureEight:
        return 1.0 / 3.0;
    }
    return 1.0;
}

bool RoomSettings::sameGeometry(const RoomSettings & other) const
{
    if (widthM != other.widthM || depthM != other.depthM || heightM != other.heightM || walls != other.walls || floor != other.floor || ceiling != other.ceiling
        || sourceX != other.sourceX || sourceY != other.sourceY || sourceSpreadM != other.sourceSpreadM) {
        return false;
    }
    for (size_t i = 0; i < MicGroupCount; i++) {
        const auto & a = groups.at(i);
        const auto & b = other.groups.at(i);
        if (a.x != b.x || a.y != b.y || a.spacingM != b.spacingM || a.angleDeg != b.angleDeg || a.pattern != b.pattern) {
            return false;
        }
    }
    return true;
}

bool RoomSettings::sameLevels(const RoomSettings & other) const
{
    for (size_t i = 0; i < MicGroupCount; i++) {
        if (groups.at(i).level != other.groups.at(i).level) {
            return false;
        }
    }
    return true;
}

RoomLayout RoomLayout::from(const RoomSettings & settings)
{
    RoomLayout layout;
    const double width = settings.widthM;
    const double depth = settings.depthM;
    const auto heightFor = [&settings](double height) {
        return std::clamp(height, 0.0, std::max(0.0, settings.heightM - ceilingMarginM));
    };

    const Vec3 centre { settings.sourceX * width, settings.sourceY * depth, heightFor(SourceHeightM) };
    const double halfSpread = settings.sourceSpreadM * 0.5;
    layout.sources[0] = { clampInside(centre.x - halfSpread, width), clampInside(centre.y, depth), centre.z };
    layout.sources[1] = { clampInside(centre.x + halfSpread, width), clampInside(centre.y, depth), centre.z };

    for (size_t g = 0; g < MicGroupCount; g++) {
        const auto & group = settings.groups.at(g);
        auto & mics = layout.mics.at(g);
        const double z = heightFor(MicHeightM.at(g));
        if (static_cast<MicGroup>(g) == MicGroup::Close) {
            // On its own source, wherever that is, offset as set
            for (size_t m = 0; m < 2; m++) {
                const auto & source = layout.sources.at(m);
                mics[m].position = { clampInside(source.x + group.x, width), clampInside(source.y + group.y, depth), z };
                mics[m].aim = aimFrom(mics[m].position, source);
                mics[m].pattern = group.pattern;
            }
            continue;
        }
        // A pair facing the middle of the source, its mics either side and turned outwards. Facing
        // the source, the right is the source's right: the right channel stays on the right.
        const Vec3 pairCentre { clampInside(group.x * width, width), clampInside(group.y * depth, depth), z };
        const Vec3 aim = aimFrom(pairCentre, centre);
        const Vec3 right { -aim.y, aim.x, 0.0 };
        const double halfSpacing = group.spacingM * 0.5;
        const double splay = group.angleDeg * std::numbers::pi / 180.0;
        for (size_t m = 0; m < 2; m++) {
            const double side = m == 0 ? -1.0 : 1.0;
            mics[m].position = { clampInside(pairCentre.x + side * right.x * halfSpacing, width), clampInside(pairCentre.y + side * right.y * halfSpacing, depth), z };
            mics[m].aim = { std::cos(splay) * aim.x + side * std::sin(splay) * right.x, std::cos(splay) * aim.y + side * std::sin(splay) * right.y, 0.0 };
            mics[m].pattern = group.pattern;
        }
    }
    return layout;
}

double volume(const RoomSettings & settings)
{
    return settings.widthM * settings.depthM * settings.heightM;
}

double surfaceArea(const RoomSettings & settings)
{
    return 2.0 * (settings.widthM * settings.depthM + settings.widthM * settings.heightM + settings.depthM * settings.heightM);
}

Bands reverbTimes(const RoomSettings & settings)
{
    const double v = volume(settings);
    const double s = surfaceArea(settings);
    const double wallArea = 2.0 * (settings.widthM + settings.depthM) * settings.heightM;
    const double floorArea = settings.widthM * settings.depthM;
    const auto & walls = absorption(settings.walls);
    const auto & floor = absorption(settings.floor);
    const auto & ceiling = absorption(settings.ceiling);
    const auto & air = airAbsorptionDbPerMetre();
    Bands times {};
    for (size_t b = 0; b < BandCount; b++) {
        const double mean = std::clamp((wallArea * walls[b] + floorArea * (floor[b] + ceiling[b])) / s, 1.0e-4, 0.99);
        // The air's intensity attenuation per metre, from decibels
        const double airPerMetre = air[b] / (10.0 * std::log10(std::numbers::e));
        times[b] = std::clamp(0.161 * v / (-s * std::log(1.0 - mean) + 4.0 * airPerMetre * v), 0.05, 20.0);
    }
    return times;
}

double midReverbTime(const Bands & reverbTimes)
{
    return (reverbTimes[2] + reverbTimes[3] + reverbTimes[4]) / 3.0;
}

double diffuseEnergyRate(double seconds, double reverbTime, double volume)
{
    // Image sources arrive at 4 pi c^3 t^2 / V a second, each at r0 / ct
    return 4.0 * std::numbers::pi * SpeedOfSound * ReferenceDistanceM * ReferenceDistanceM / volume * std::exp(-sixtyDbNepers * seconds / reverbTime);
}

void ImpulseDesigner::Biquad::setLowPass(double hz, double sampleRate)
{
    // Butterworth, by the bilinear transform
    const double k = std::tan(std::numbers::pi * std::min(hz, sampleRate * 0.45) / sampleRate);
    const double q = std::numbers::sqrt2 * 0.5;
    const double norm = 1.0 / (1.0 + k / q + k * k);
    b0 = k * k * norm;
    b1 = 2.0 * b0;
    b2 = b0;
    a1 = 2.0 * (k * k - 1.0) * norm;
    a2 = (1.0 - k / q + k * k) * norm;
    z1 = 0.0;
    z2 = 0.0;
}

double ImpulseDesigner::Biquad::process(double input)
{
    // Transposed direct form II
    const double output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    return output;
}

void ImpulseDesigner::configure(double sampleRate)
{
    m_sampleRate = sampleRate;
    m_capacity = static_cast<size_t>(std::ceil(MaxWindowSeconds * sampleRate)) + 4;
    for (auto && buffer : m_bandBuffers) {
        buffer.assign(m_capacity, 0.0);
    }
    for (auto && response : m_responses) {
        response.assign(m_capacity, 0.0);
    }
    for (auto && images : m_images) {
        images.clear();
        images.reserve(MaxImages);
    }
    for (auto && slot : m_reflectionPowers) {
        for (auto && powers : slot) {
            powers.assign(maxBounces + 1, 0.0);
        }
    }
    m_stage = Stage::Idle;
    m_length = 0;
}

bool ImpulseDesigner::isConfigured() const
{
    return m_capacity > 0;
}

void ImpulseDesigner::start(const RoomSettings & settings)
{
    if (!isConfigured()) {
        return;
    }
    m_settings = settings;
    prepare();
}

bool ImpulseDesigner::isDesigning() const
{
    return m_stage != Stage::Idle && m_stage != Stage::Done;
}

void ImpulseDesigner::prepare()
{
    m_layout = RoomLayout::from(m_settings);
    m_reverbTimes = RoomAcoustics::reverbTimes(m_settings);

    // The window covers the furthest direct sound and what follows it, as long as the images it
    // takes fit: a small room fills with them quickly
    double furthest = 0.0;
    for (auto && pair : m_layout.mics) {
        for (auto && mic : pair) {
            for (auto && source : m_layout.sources) {
                furthest = std::max(furthest, std::hypot(mic.position.x - source.x, mic.position.y - source.y, mic.position.z - source.z));
            }
        }
    }
    const double directSeconds = furthest / SpeedOfSound;
    const double v = volume(m_settings);
    const double halfDiagonal = 0.5 * std::hypot(m_settings.widthM, m_settings.depthM, m_settings.heightM);
    const double imageRadius = std::cbrt(0.8 * static_cast<double>(MaxImages) * 3.0 * v / (4.0 * std::numbers::pi));
    const double fitSeconds = (imageRadius - halfDiagonal) / SpeedOfSound;
    m_window = std::min({ directSeconds + WindowAfterDirectSeconds, fitSeconds, MaxWindowSeconds });
    m_window = std::clamp(m_window, std::min(directSeconds + 0.02, MaxWindowSeconds), MaxWindowSeconds);
    m_crossover = std::min(CrossoverSeconds, 0.5 * (m_window - directSeconds));
    m_length = std::min(m_capacity - 4, static_cast<size_t>(std::ceil(m_window * m_sampleRate)));

    // What n bounces off each kind of surface leave, per band, as an amplitude
    const std::array<Material, 3> materials { m_settings.walls, m_settings.floor, m_settings.ceiling };
    for (size_t slot = 0; slot < materials.size(); slot++) {
        const auto & alpha = absorption(materials[slot]);
        for (size_t b = 0; b < BandCount; b++) {
            const double perBounce = std::sqrt(1.0 - alpha[b]);
            auto && powers = m_reflectionPowers[slot][b];
            double value = 1.0;
            for (size_t n = 0; n <= maxBounces; n++) {
                powers[n] = value;
                value *= perBounce;
            }
        }
    }
    const auto & air = airAbsorptionDbPerMetre();
    for (size_t b = 0; b < BandCount; b++) {
        m_airNepersPerMetre[b] = air[b] * std::numbers::ln10 / 20.0;
    }
    for (size_t b = 0; b < m_crossovers.size(); b++) {
        m_crossovers[b].setLowPass(crossoverHz[b], m_sampleRate);
    }
    for (auto && buffer : m_bandBuffers) {
        std::fill(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(m_length + 4), 0.0);
    }
    for (auto && images : m_images) {
        images.clear();
    }

    const double radius = m_window * SpeedOfSound + halfDiagonal;
    m_slabCount = static_cast<int>(std::ceil((radius + m_settings.widthM) / (2.0 * m_settings.widthM)));
    m_slab = -m_slabCount;
    m_source = 0;
    m_response = 0;
    m_cursor = 0;
    m_stage = Stage::Enumerate;
}

void ImpulseDesigner::enumerateSlab()
{
    // One value of nx: every image whose x is the source's mirrored nx times in the side walls
    const auto & source = m_layout.sources.at(m_source);
    const double width = m_settings.widthM;
    const double depth = m_settings.depthM;
    const double height = m_settings.heightM;
    const Vec3 centre { width * 0.5, depth * 0.5, height * 0.5 };
    const double halfDiagonal = 0.5 * std::hypot(width, depth, height);
    const double radius = m_window * SpeedOfSound + halfDiagonal;
    const int ny = static_cast<int>(std::ceil((radius + depth) / (2.0 * depth)));
    const int nz = static_cast<int>(std::ceil((radius + height) / (2.0 * height)));
    auto && images = m_images.at(m_source);
    const int nx = m_slab;
    for (int px = 0; px < 2; px++) {
        const double x = (1 - 2 * px) * source.x + 2.0 * nx * width;
        const double dx = x - centre.x;
        if (std::abs(dx) > radius) {
            continue;
        }
        const auto xHits = static_cast<size_t>(std::abs(nx - px) + std::abs(nx));
        for (int j = -ny; j <= ny; j++) {
            for (int py = 0; py < 2; py++) {
                const double y = (1 - 2 * py) * source.y + 2.0 * j * depth;
                const double dy = y - centre.y;
                if (dx * dx + dy * dy > radius * radius) {
                    continue;
                }
                const auto wallHits = std::min(maxBounces, xHits + static_cast<size_t>(std::abs(j - py) + std::abs(j)));
                for (int k = -nz; k <= nz; k++) {
                    for (int pz = 0; pz < 2; pz++) {
                        const double z = (1 - 2 * pz) * source.z + 2.0 * k * height;
                        const double dz = z - centre.z;
                        if (dx * dx + dy * dy + dz * dz > radius * radius || images.size() >= MaxImages) {
                            continue;
                        }
                        const auto floorHits = std::min(maxBounces, static_cast<size_t>(std::abs(k - pz)));
                        const auto ceilingHits = std::min(maxBounces, static_cast<size_t>(std::abs(k)));
                        Image image;
                        image.position = { x, y, z };
                        for (size_t b = 0; b < BandCount; b++) {
                            image.reflection[b] = m_reflectionPowers[0][b][wallHits] * m_reflectionPowers[1][b][floorHits] * m_reflectionPowers[2][b][ceilingHits];
                        }
                        images.push_back(image);
                    }
                }
            }
        }
    }
}

void ImpulseDesigner::deposit(size_t band, double delaySamples, double value)
{
    // Cubic Lagrange, so that a reflection lands between samples without losing its top
    auto && buffer = m_bandBuffers[band];
    const auto whole = static_cast<std::ptrdiff_t>(std::floor(delaySamples));
    const double mu = delaySamples - static_cast<double>(whole);
    const auto end = static_cast<std::ptrdiff_t>(m_length);
    if (mu < 1.0e-9) {
        if (whole >= 0 && whole < end) {
            buffer[static_cast<size_t>(whole)] += value;
        }
        return;
    }
    const std::array<double, 4> weights {
        -mu * (mu - 1.0) * (mu - 2.0) / 6.0,
        (mu + 1.0) * (mu - 1.0) * (mu - 2.0) / 2.0,
        -(mu + 1.0) * mu * (mu - 2.0) / 2.0,
        (mu + 1.0) * mu * (mu - 1.0) / 6.0
    };
    for (std::ptrdiff_t i = 0; i < 4; i++) {
        if (const auto index = whole - 1 + i; index >= 0 && index < end) {
            buffer[static_cast<size_t>(index)] += value * weights[static_cast<size_t>(i)];
        }
    }
}

void ImpulseDesigner::depositImages(size_t count)
{
    // Response index: (group * 2 + source) * 2 + mic
    const size_t mic = m_response % 2;
    const size_t source = (m_response / 2) % SourceCount;
    const size_t group = m_response / (2 * SourceCount);
    const auto & receiver = m_layout.mics.at(group).at(mic);
    const auto & images = m_images.at(source);
    const size_t end = std::min(images.size(), m_cursor + count);
    const double maxDistance = m_window * SpeedOfSound;
    for (; m_cursor < end; m_cursor++) {
        const auto & image = images[m_cursor];
        const double dx = image.position.x - receiver.position.x;
        const double dy = image.position.y - receiver.position.y;
        const double dz = image.position.z - receiver.position.z;
        const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance > maxDistance) {
            continue;
        }
        const double cosine = distance > 1.0e-9 ? (dx * receiver.aim.x + dy * receiver.aim.y + dz * receiver.aim.z) / distance : 1.0;
        const double gain = ReferenceDistanceM / std::sqrt(distance * distance + ReferenceDistanceM * ReferenceDistanceM) * patternGain(receiver.pattern, cosine);
        if (std::abs(gain) < 1.0e-9) {
            continue;
        }
        Bands bands {};
        for (size_t b = 0; b < BandCount; b++) {
            bands[b] = gain * image.reflection[b] * std::exp(-m_airNepersPerMetre[b] * distance);
        }
        // The bank sums low-passed differences: each band's gain against the next one up
        const double delay = distance / SpeedOfSound * m_sampleRate;
        for (size_t b = 0; b + 1 < BandCount; b++) {
            deposit(b, delay, bands[b] - bands[b + 1]);
        }
        deposit(BandCount - 1, delay, bands[BandCount - 1]);
    }
}

void ImpulseDesigner::filterSamples(size_t count)
{
    // y = sum of LP_b(d_b) + d_top: each crossover hands one band over to the next, and with every
    // band at the same gain the differences vanish and the bank passes the response whole
    auto && response = m_responses[m_response];
    const size_t end = std::min(m_length, m_cursor + count);
    for (; m_cursor < end; m_cursor++) {
        double sum = m_bandBuffers[BandCount - 1][m_cursor];
        m_bandBuffers[BandCount - 1][m_cursor] = 0.0;
        for (size_t b = 0; b + 1 < BandCount; b++) {
            sum += m_crossovers[b].process(m_bandBuffers[b][m_cursor]);
            m_bandBuffers[b][m_cursor] = 0.0;
        }
        response[m_cursor] = sum;
    }
}

void ImpulseDesigner::finishResponse()
{
    // Out over the crossover, where the tail comes in
    auto && response = m_responses[m_response];
    const double fadeStart = (m_window - m_crossover) * m_sampleRate;
    const double fadeLength = std::max(1.0, m_crossover * m_sampleRate);
    double energy = 0.0;
    for (size_t n = 0; n < m_length; n++) {
        if (const double position = static_cast<double>(n) - fadeStart; position > 0.0) {
            const double t = std::min(1.0, position / fadeLength);
            response[n] *= 0.5 * (1.0 + std::cos(std::numbers::pi * t));
        }
        energy += response[n] * response[n];
    }
    // Clear what the deposit spilled past the end, so the next response starts clean
    for (auto && buffer : m_bandBuffers) {
        std::fill(buffer.begin() + static_cast<std::ptrdiff_t>(m_length), buffer.begin() + static_cast<std::ptrdiff_t>(m_length + 4), 0.0);
    }
    for (auto && crossover : m_crossovers) {
        crossover.z1 = 0.0;
        crossover.z2 = 0.0;
    }
    m_energies[m_response] = energy;
}

void ImpulseDesigner::computeNormalization()
{
    // Each pair to the level of a mic on its source: the mic on each channel's own source, the
    // early part as designed and the tail as the room's decay predicts it
    const double midTime = midReverbTime(m_reverbTimes);
    const double lateStart = m_window - 0.5 * m_crossover;
    const double lateEnergy = diffuseEnergyRate(lateStart, midTime, volume(m_settings)) * midTime / sixtyDbNepers;
    for (size_t g = 0; g < MicGroupCount; g++) {
        const auto group = static_cast<MicGroup>(g);
        double energy = 0.0;
        for (size_t m = 0; m < 2; m++) {
            energy += m_energies[responseIndex(group, m, m)];
        }
        energy = energy * 0.5 + lateEnergy * randomEnergyEfficiency(m_settings.groups.at(g).pattern);
        m_normalization[g] = 1.0 / std::sqrt(std::max(energy, 1.0e-9));
    }
}

bool ImpulseDesigner::advance(double work)
{
    if (!isDesigning()) {
        return false;
    }
    while (work > 0.0) {
        switch (m_stage) {
        case Stage::Enumerate: {
            const auto before = m_images.at(m_source).size();
            enumerateSlab();
            // The candidates looked at, which is most of the cost, and the images kept
            const double width = m_settings.widthM;
            const double radius = m_window * SpeedOfSound + 0.5 * std::hypot(width, m_settings.depthM, m_settings.heightM);
            const double candidates = 8.0 * std::ceil((radius + m_settings.depthM) / m_settings.depthM) * std::ceil((radius + m_settings.heightM) / m_settings.heightM);
            work -= candidates * enumerateCandidateWork + static_cast<double>(m_images.at(m_source).size() - before);
            if (++m_slab > m_slabCount) {
                m_slab = -m_slabCount;
                if (++m_source == SourceCount) {
                    m_stage = Stage::Deposit;
                    m_response = 0;
                    m_cursor = 0;
                }
            }
            break;
        }
        case Stage::Deposit: {
            const size_t source = (m_response / 2) % SourceCount;
            const size_t count = std::max<size_t>(1, static_cast<size_t>(work));
            const size_t before = m_cursor;
            depositImages(count);
            work -= static_cast<double>(m_cursor - before);
            if (m_cursor >= m_images.at(source).size()) {
                m_stage = Stage::Filter;
                m_cursor = 0;
            }
            break;
        }
        case Stage::Filter: {
            const size_t count = std::max<size_t>(64, static_cast<size_t>(work / filterSampleWork));
            const size_t before = m_cursor;
            filterSamples(count);
            work -= static_cast<double>(m_cursor - before) * filterSampleWork;
            if (m_cursor >= m_length) {
                finishResponse();
                work -= static_cast<double>(m_length) * filterSampleWork * 0.2;
                m_cursor = 0;
                if (++m_response == ResponseCount) {
                    computeNormalization();
                    m_stage = Stage::Done;
                    return true;
                }
                m_stage = Stage::Deposit;
            }
            break;
        }
        case Stage::Idle:
        case Stage::Done:
            return false;
        }
    }
    return false;
}

void ImpulseDesigner::finish()
{
    while (isDesigning()) {
        advance(1.0e9);
    }
}

size_t ImpulseDesigner::responseIndex(MicGroup group, size_t source, size_t mic)
{
    return (static_cast<size_t>(group) * SourceCount + source) * 2 + mic;
}

const std::vector<double> & ImpulseDesigner::response(size_t index) const
{
    return m_responses.at(index);
}

size_t ImpulseDesigner::length() const
{
    return m_length;
}

double ImpulseDesigner::normalization(MicGroup group) const
{
    return m_normalization.at(static_cast<size_t>(group));
}

double ImpulseDesigner::windowSeconds() const
{
    return m_window;
}

double ImpulseDesigner::crossoverSeconds() const
{
    return m_crossover;
}

const Bands & ImpulseDesigner::reverbTimes() const
{
    return m_reverbTimes;
}

const RoomSettings & ImpulseDesigner::settings() const
{
    return m_settings;
}

} // namespace noteahead::RoomAcoustics
