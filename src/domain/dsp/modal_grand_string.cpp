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

#include "modal_grand_string.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace noteahead {

namespace {

constexpr double Pi = 3.14159265358979323846;

//! C4, where the decay laws are quoted
constexpr double ReferenceHz = 261.63;

// The hammer, the strike point and the soundboard, fitted jointly to the recording's attack spectra
constexpr double HammerHz = 860.0;
constexpr double HammerPoles = 2.57;
constexpr double StrikeBass = 0.121; //!< Of the string's length, at A0
constexpr double StrikeTreble = 0.110; //!< At C8
//! How shallow the strike point's nulls are: a hammer is wide, and the strings are not quite still
//! where it lands
constexpr double StrikeFill = 0.351;
constexpr double RadiationHz = 132.0;
constexpr double RadiationQ = 4.72;

// The decay, fitted through the partials above to every note's envelope over the 1.8 s it was held,
// and to how much darker each note is after 1.5 s than in its attack. To 1.4 dB a note
constexpr double SlowDecayAtC4 = 9.67;
constexpr double SlowDecayExponent = 0.255;
constexpr double FastDecayAtC4 = 8.59;
constexpr double FastDecayExponent = 1.783;
//! Where the fast part is half its most, in octaves above C4, how steeply it takes over, and the
//! most it takes: even the top keys keep a slow part of their own
constexpr double FastShareMidpoint = 0.66;
constexpr double FastShareSteepness = 2.58;
constexpr double MaxFastShare = 0.93;
//! How much faster each hertz of a partial dies, per second
constexpr double LossPerHz = 4.28e-4;

//! The velocity the recording was played at, where every law above holds as fitted
constexpr double ReferenceVelocity = 100.0 / 127.0;
//! How the hammer hardens as it is thrown harder: felt stiffens under compression
constexpr double HammerVelocityExponent = 0.85;
constexpr double LevelVelocityExponent = 1.8;
//! The soft pedal moves the hammers onto fresher felt
constexpr double SoftPedalHammer = 0.7;
constexpr double SoftPedalLevel = 0.7;

constexpr double MaxPartialHz = 18000.0;
//! Partials quieter than this against the strongest are not worth a resonator
constexpr double MinPartialLevel = 1.0e-4;
constexpr double MaxUnisonCents = 4.0;

constexpr double DamperDelaySeconds = 0.008;
constexpr double MinDamperSeconds = 0.04;
constexpr double MaxDamperSeconds = 1.0;
//! A damper's felt takes the upper partials faster
constexpr double DamperHzCorner = 2000.0;

constexpr double ModeSilence = 1.0e-7;
constexpr double Silence = 1.0e-10;
constexpr int PruneInterval = 512;

struct Anchor
{
    int note;
    double value;
};

// The recording's tuning, in cents from equal temperament with A4 at 0, every third key
constexpr std::array<Anchor, 31> StretchCents { { { 21, -11.0 }, { 22, -10.0 }, { 25, -7.3 }, { 28, -5.5 }, { 31, -4.0 }, { 34, -4.0 }, { 37, -3.8 }, { 40, -3.3 }, { 43, -3.4 }, { 46, -2.6 }, { 49, -2.3 }, { 52, -2.1 }, { 55, -2.0 }, { 58, -1.5 }, { 61, -1.4 }, { 64, -0.5 }, { 67, -0.2 }, { 70, 0.0 }, { 73, 0.0 }, { 76, 0.4 }, { 79, 0.3 }, { 82, 0.9 }, { 85, 2.5 }, { 88, 3.5 }, { 91, 7.6 }, { 94, 14.1 }, { 97, 18.2 }, { 100, 21.1 }, { 103, 16.2 }, { 106, 9.7 }, { 108, 8.4 } } };

// B, measured up to E6 and carried on above it at the rate it rises there
constexpr std::array<Anchor, 31> Inharmonicity { { { 21, 8.9e-5 }, { 22, 8.91e-5 }, { 25, 8.77e-5 }, { 28, 5.78e-5 }, { 31, 5.22e-5 }, { 34, 4.99e-5 }, { 37, 5.2e-5 }, { 40, 6.38e-5 }, { 43, 7.86e-5 }, { 46, 9.24e-5 }, { 49, 1.24e-4 }, { 52, 1.5e-4 }, { 55, 2.18e-4 }, { 58, 2.63e-4 }, { 61, 3.56e-4 }, { 64, 4.4e-4 }, { 67, 5.96e-4 }, { 70, 8.18e-4 }, { 73, 1.03e-3 }, { 76, 1.42e-3 }, { 79, 2.09e-3 }, { 82, 2.73e-3 }, { 85, 3.49e-3 }, { 88, 4.29e-3 }, { 91, 5.3e-3 }, { 94, 6.6e-3 }, { 97, 8.2e-3 }, { 100, 1.0e-2 }, { 103, 1.25e-2 }, { 106, 1.55e-2 }, { 108, 1.8e-2 } } };

// What each key needs, in dB, to peak where the recording has it at velocity 100, at -12 dBFS on
// average: the recording is flat from the bass to C7 and falls by 8 dB over the top octave, where
// the model alone would fall by 9 dB from the bass up. Smoothed over three anchors, so that the
// microphones' key-to-key scatter is not copied
constexpr std::array<Anchor, 30> KeyLevelDb { { { 22, -4.8 }, { 25, -4.8 }, { 28, -5.1 }, { 31, -4.6 }, { 34, -4.4 }, { 37, -4.7 }, { 40, -5.3 }, { 43, -5.0 }, { 46, -3.7 }, { 49, -3.0 }, { 52, -2.8 }, { 55, -2.9 }, { 58, -2.1 }, { 61, -1.0 }, { 64, -0.2 }, { 67, -0.1 }, { 70, -0.4 }, { 73, 0.2 }, { 76, 1.3 }, { 79, 2.6 }, { 82, 2.7 }, { 85, 2.3 }, { 88, 2.0 }, { 91, 2.8 }, { 94, 2.7 }, { 97, 2.3 }, { 100, 0.2 }, { 103, -1.5 }, { 106, -3.2 }, { 108, -3.8 } } };

template<size_t N>
double interpolate(const std::array<Anchor, N> & anchors, int note, bool geometric)
{
    if (note <= anchors.front().note) {
        return anchors.front().value;
    }
    for (size_t i = 1; i < N; i++) {
        if (note <= anchors.at(i).note) {
            const auto & a = anchors.at(i - 1);
            const auto & b = anchors.at(i);
            const double t = static_cast<double>(note - a.note) / static_cast<double>(b.note - a.note);
            return geometric ? a.value * std::pow(b.value / a.value, t) : a.value + (b.value - a.value) * t;
        }
    }
    return anchors.back().value;
}

//! A number in [-1, 1] that is always the same for the same key, partial and string: where the
//! microphones happen to hear each mode, and how a key's strings happen to be tuned
double scatter(int note, int partial, int string)
{
    auto h = static_cast<uint32_t>(note * 73856093) ^ static_cast<uint32_t>(partial * 19349663) ^ static_cast<uint32_t>(string * 83492791);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<double>(h & 0xffffu) / 32767.5 - 1.0;
}

double noteHz(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

} // namespace

double ModalGrandString::stretchCents(int note)
{
    return interpolate(StretchCents, note, false);
}

double ModalGrandString::inharmonicity(int note)
{
    return interpolate(Inharmonicity, note, true);
}

int ModalGrandString::stringCount(int note)
{
    if (note < 30) {
        return 1;
    }
    return note < 42 ? 2 : 3;
}

double ModalGrandString::slowDecaySeconds(double frequency)
{
    return SlowDecayAtC4 * std::pow(frequency / ReferenceHz, -SlowDecayExponent);
}

double ModalGrandString::fastDecaySeconds(double frequency)
{
    // In the bass the fit's fast decay runs past the slow one, where the fast part is nothing anyway
    return std::min(FastDecayAtC4 * std::pow(frequency / ReferenceHz, -FastDecayExponent), slowDecaySeconds(frequency));
}

double ModalGrandString::fastShare(double frequency)
{
    return MaxFastShare / (1.0 + std::exp(-(std::log2(frequency / ReferenceHz) - FastShareMidpoint) * FastShareSteepness));
}

double ModalGrandString::partialLevel(int note, int partial, double frequency, double hammerHz)
{
    const double strike = StrikeBass + (StrikeTreble - StrikeBass) * (note - 21) / 87.0;
    const double node = std::sin(partial * Pi * strike);
    const double comb = std::sqrt(node * node + StrikeFill * StrikeFill);
    const double hammer = 1.0 / std::sqrt(1.0 + std::pow(frequency / hammerHz, 2.0 * HammerPoles));
    const double w = frequency / RadiationHz;
    const double radiation = w * w / std::sqrt((1.0 - w * w) * (1.0 - w * w) + (w / RadiationQ) * (w / RadiationQ));
    return comb * hammer * radiation;
}

double ModalGrandString::keyPan(int note, double panSpread)
{
    // As the player hears it. The default, 0.6, puts the treble 8 dB to the right, as the recording has it
    return std::clamp(0.5 + 0.5 * panSpread * std::tanh((note - 60) / 18.0), 0.0, 1.0);
}

void ModalGrandString::setSampleRate(double sampleRate)
{
    if (sampleRate == m_sampleRate || sampleRate <= 0.0) {
        return;
    }
    m_sampleRate = sampleRate;
    for (int i = 0; i < m_modeCount; i++) {
        setPole(i, m_modes.sigma.at(i));
    }
}

void ModalGrandString::setPole(int mode, double sigma)
{
    const double omega = 2.0 * Pi * m_modes.omega.at(mode) / m_sampleRate;
    const double r = std::exp(-sigma / m_sampleRate);
    m_modes.sigma.at(mode) = sigma;
    m_modes.a1.at(mode) = 2.0 * r * std::cos(omega);
    m_modes.a2.at(mode) = -r * r;
}

void ModalGrandString::addMode(double frequency, double sigma, double amplitude, double pan, double spread)
{
    if (m_modeCount >= MaxModes || frequency >= 0.45 * m_sampleRate) {
        return;
    }
    const int i = m_modeCount++;
    m_modes.omega.at(i) = frequency;
    setPole(i, sigma);
    // Each mode opens on its own rising zero crossing, as a string set moving by a blow does
    const double omega = 2.0 * Pi * frequency / m_sampleRate;
    const double r = std::exp(-sigma / m_sampleRate);
    m_modes.y1.at(i) = 0.0;
    m_modes.y2.at(i) = -std::sin(omega) / (r * r);
    // Equal power across the microphones, and each mode heard a little more by one than the other
    const double angle = std::clamp(pan, 0.0, 1.0) * 0.5 * Pi;
    m_modes.left.at(i) = amplitude * std::cos(angle) * (1.0 + spread);
    m_modes.right.at(i) = amplitude * std::sin(angle) * (1.0 - spread);
}

void ModalGrandString::strike(uint8_t note, float velocity, const Settings & settings, bool softPedal)
{
    m_note = note;
    m_modeCount = 0;
    m_damped = false;
    m_damperCountdown = 0;
    m_energy = 0.0;

    const double v = std::clamp(static_cast<double>(velocity), 0.01, 1.0) / ReferenceVelocity;
    double hammerHz = HammerHz * std::exp2(3.0 * (static_cast<double>(settings.hardness) - 0.5)) * std::pow(v, HammerVelocityExponent);
    double level = 1.0 - static_cast<double>(settings.velocitySensitivity) * (1.0 - std::pow(v, LevelVelocityExponent));
    if (softPedal) {
        hammerHz *= SoftPedalHammer;
        level *= SoftPedalLevel;
    }

    const int n = static_cast<int>(note);
    const double cents = stretchCents(n) * static_cast<double>(settings.stretch);
    const double f0 = noteHz(n) * std::exp2(cents / 1200.0);
    const double b = inharmonicity(n) * 2.0 * static_cast<double>(settings.inharmonicity);
    const double decayScale = std::exp2(2.0 * (static_cast<double>(settings.decay) - 0.5));
    const double lossScale = std::pow(4.0, 1.0 - 2.0 * static_cast<double>(settings.brightness));
    const double slowSigma = 6.91 / (slowDecaySeconds(f0) * decayScale);
    const double fastSigma = 6.91 / (fastDecaySeconds(f0) * decayScale);
    const double share = fastShare(f0);
    const int strings = stringCount(n);
    // Squared, so the default is the third of a cent a concert grand's unisons are kept to, which beats
    // too slowly to be heard as anything but life in the tail, and the top of the knob is a bar piano
    const double unisonCents = MaxUnisonCents * static_cast<double>(settings.detune * settings.detune);

    // The partials' levels first, to bring the note to one energy whatever its spectrum
    const double limit = std::min(MaxPartialHz, 0.45 * m_sampleRate);
    std::array<double, MaxPartials> frequencies {};
    std::array<double, MaxPartials> levels {};
    int partials = 0;
    double strongest = 0.0;
    double energy = 0.0;
    for (int k = 1; k <= MaxPartials; k++) {
        // Stiffness pulls the fundamental sharp too, so the series is divided by its own first term
        const double f = k * f0 * std::sqrt((1.0 + b * k * k) / (1.0 + b));
        if (f >= limit) {
            break;
        }
        frequencies.at(partials) = f;
        levels.at(partials) = partialLevel(n, k, f, hammerHz);
        strongest = std::max(strongest, levels.at(partials));
        energy += levels.at(partials) * levels.at(partials);
        partials++;
    }
    if (!partials) {
        m_active = false;
        return;
    }

    const double gain = level * std::pow(10.0, interpolate(KeyLevelDb, n, false) / 20.0) / std::sqrt(energy);
    const double pan = keyPan(n, static_cast<double>(settings.panSpread));
    const double spread = 0.15 * static_cast<double>(settings.panSpread);
    for (int i = 0; i < partials; i++) {
        if (levels.at(i) < MinPartialLevel * strongest) {
            continue;
        }
        const int k = i + 1;
        const double f = frequencies.at(i);
        const double amplitude = levels.at(i) * gain;
        const double loss = LossPerHz * lossScale * f;
        const double slow = slowSigma + loss;
        const double fast = fastSigma + loss;
        if (k <= UnisonPartials) {
            for (int s = 0; s < strings; s++) {
                // The middle string on pitch, the others either side of it
                const double offset = s == 0 ? 0.0 : (s == 1 ? 1.0 : -1.0) * unisonCents * (0.6 + 0.4 * std::abs(scatter(n, 0, s)));
                addMode(f * std::exp2(offset / 1200.0), slow, amplitude * (1.0 - share) / strings, pan, spread * scatter(n, k, s));
            }
            if (share > 0.01) {
                addMode(f, fast, amplitude * share, pan, spread * scatter(n, k, MaxStrings));
            }
        } else {
            // One mode with the energy-weighted decay of the two parts
            const double integral = share * share / (2.0 * fast) + (1.0 - share) * (1.0 - share) / (2.0 * slow) + 2.0 * share * (1.0 - share) / (fast + slow);
            addMode(f, 1.0 / (2.0 * integral), amplitude, pan, spread * scatter(n, k, 0));
        }
    }

    const double damperSeconds = MinDamperSeconds * std::pow(MaxDamperSeconds / MinDamperSeconds, static_cast<double>(settings.release)) * std::pow(ReferenceHz / f0, 0.3);
    m_damperSigma = 6.91 / damperSeconds;
    m_attackPhase = 0.0;
    m_attackStep = 1.0 / std::max(1.0, 0.6 / hammerHz * m_sampleRate);
    m_pruneCounter = 0;
    m_active = m_modeCount > 0;
}

void ModalGrandString::damp()
{
    if (!m_active || m_damped || m_note >= LowestUndampedNote) {
        return;
    }
    m_damped = true;
    m_damperCountdown = std::max(1, static_cast<int>(DamperDelaySeconds * m_sampleRate));
}

void ModalGrandString::process(double & left, double & right)
{
    if (!m_active) {
        return;
    }

    if (m_damperCountdown > 0 && --m_damperCountdown == 0) {
        for (int i = 0; i < m_modeCount; i++) {
            setPole(i, m_modes.sigma.at(i) + m_damperSigma * (1.0 + m_modes.omega.at(i) / DamperHzCorner));
        }
    }

    double l = 0.0;
    double r = 0.0;
    auto & m = m_modes;
    for (int i = 0; i < m_modeCount; i++) {
        const double y = m.a1[i] * m.y1[i] + m.a2[i] * m.y2[i];
        m.y2[i] = m.y1[i];
        m.y1[i] = y;
        l += m.left[i] * y;
        r += m.right[i] * y;
    }

    if (m_attackPhase < 1.0) {
        const double ramp = 0.5 - 0.5 * std::cos(Pi * m_attackPhase);
        l *= ramp;
        r *= ramp;
        m_attackPhase += m_attackStep;
    }

    left += l;
    right += r;
    m_energy += 0.001 * (0.5 * (l * l + r * r) - m_energy);

    if (++m_pruneCounter >= PruneInterval) {
        m_pruneCounter = 0;
        // A mode that has died is swapped out for the last one, so the loop only ever runs the live
        for (int i = 0; i < m_modeCount;) {
            const double size = std::max(std::abs(m.y1[i]), std::abs(m.y2[i])) * std::max(std::abs(m.left[i]), std::abs(m.right[i]));
            if (size < ModeSilence) {
                const int last = --m_modeCount;
                m.a1[i] = m.a1[last];
                m.a2[i] = m.a2[last];
                m.y1[i] = m.y1[last];
                m.y2[i] = m.y2[last];
                m.left[i] = m.left[last];
                m.right[i] = m.right[last];
                m.omega[i] = m.omega[last];
                m.sigma[i] = m.sigma[last];
            } else {
                i++;
            }
        }
        m_active = m_modeCount > 0 && (m_energy > Silence || m_attackPhase < 1.0);
    }
}

bool ModalGrandString::isActive() const
{
    return m_active;
}

uint8_t ModalGrandString::note() const
{
    return m_note;
}

double ModalGrandString::energy() const
{
    return m_energy;
}

int ModalGrandString::modeCount() const
{
    return m_modeCount;
}

void ModalGrandString::reset()
{
    m_modeCount = 0;
    m_active = false;
    m_damped = false;
    m_damperCountdown = 0;
    m_energy = 0.0;
    m_attackPhase = 1.0;
}

} // namespace noteahead
