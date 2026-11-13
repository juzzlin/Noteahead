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

#include "room_mics.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

using namespace RoomAcoustics;

//! The design's work per frame of audio: an image reflected into one response is one, some 65 ns.
//! A design is 20 000 to 70 000 of them, so a room is redesigned within 30 to 90 ms of moving
//! anything, at about four percent of a core while it runs.
constexpr double designWorkPerFrame { 16.0 };
//! The partitions loaded per frame: a transform each.
constexpr double loadPartitionsPerFrame { 1.0 / 8.0 };

std::string key(const QString & name)
{
    return name.toStdString();
}

const std::array<RoomSettings::Group, MicGroupCount> & defaultGroups()
{
    static const std::array<RoomSettings::Group, MicGroupCount> groups { {
      { .x = 0.0, .y = 0.0, .spacingM = 0.0, .angleDeg = 0.0, .pattern = Pattern::Cardioid, .level = 1.0 },
      { .x = 0.5, .y = 0.62, .spacingM = 1.0, .angleDeg = 0.0, .pattern = Pattern::Omni, .level = 0.5 },
      { .x = 0.5, .y = 0.9, .spacingM = 3.0, .angleDeg = 0.0, .pattern = Pattern::Omni, .level = 0.3 },
    } };
    return groups;
}

RoomSettings defaultSettings()
{
    RoomSettings settings;
    settings.groups = defaultGroups();
    return settings;
}

float levelPosition(double gain)
{
    return gain <= 0.0 ? 0.0f : static_cast<float>(ParameterMapper::unmapFader(gain));
}

int xmlPosition(float position)
{
    return static_cast<int>(std::lround(position * 10000.0f));
}

} // namespace

RoomMics::RoomMics()
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    const auto defaults = parametersFor(defaultSettings(), 1.0f);
    const auto continuous = [this, &defaults](const QString & name) {
        const float position = defaults.at(key(name));
        addParameter(Parameter { key(name), position, 0, 10000, xmlPosition(position), 100 });
    };
    const auto discrete = [this, &defaults](const QString & name, size_t count) {
        const float index = defaults.at(key(name));
        addParameter(Parameter { key(name), index, 0, static_cast<int>(count) - 1, static_cast<int>(index), 1, Parameter::Type::Discrete });
    };

    continuous(C::xmlKeyRoomWidth());
    continuous(C::xmlKeyRoomDepth());
    continuous(C::xmlKeyRoomHeight());
    discrete(C::xmlKeyWallMaterial(), MaterialCount);
    discrete(C::xmlKeyFloorMaterial(), MaterialCount);
    discrete(C::xmlKeyCeilingMaterial(), MaterialCount);
    continuous(C::xmlKeySourceX());
    continuous(C::xmlKeySourceY());
    continuous(C::xmlKeySpread());
    for (size_t g = 0; g < MicGroupCount; g++) {
        continuous(C::xmlKeyMicX(g));
        continuous(C::xmlKeyMicY(g));
        // The Close pair stands on the sources, as far apart as they are, facing each its own
        if (static_cast<MicGroup>(g) != MicGroup::Close) {
            continuous(C::xmlKeyMicSpacing(g));
            continuous(C::xmlKeyMicAngle(g));
        }
        discrete(C::xmlKeyMicPattern(g), PatternCount);
        continuous(C::xmlKeyMicLevel(g));
    }

    // The mics replace the signal: the Close pair at its default is the signal, with the room
    addMixParameter(1.0f, MixLaw::Crossfade, 0, 10000, 100);
    addSoloParameter();

    const auto describe = [this](const QString & name, Display display) {
        setParameterDisplay(key(name), std::move(display));
    };
    describe(C::xmlKeyRoomWidth(), { .mapping = Display::Mapping::Exponential, .unit = "m", .min = MinSizeM, .max = MaxSizeM, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Width") });
    describe(C::xmlKeyRoomDepth(), { .mapping = Display::Mapping::Exponential, .unit = "m", .min = MinSizeM, .max = MaxSizeM, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Depth") });
    describe(C::xmlKeyRoomHeight(), { .mapping = Display::Mapping::Exponential, .unit = "m", .min = MinSizeM, .max = MaxHeightM, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Height") });
    describe(C::xmlKeySourceX(), { .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Source X") });
    describe(C::xmlKeySourceY(), { .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Source Y") });
    describe(C::xmlKeySpread(), { .unit = "m", .min = 0.0, .max = MaxSpreadM, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Spread") });
    const std::array<const char *, MicGroupCount> levelLabels { QT_TRANSLATE_NOOP("ParameterDisplay", "Close Level"), QT_TRANSLATE_NOOP("ParameterDisplay", "Room Level"), QT_TRANSLATE_NOOP("ParameterDisplay", "Far Level") };
    const std::array<const char *, MicGroupCount> xLabels { QT_TRANSLATE_NOOP("ParameterDisplay", "Close X"), QT_TRANSLATE_NOOP("ParameterDisplay", "Room X"), QT_TRANSLATE_NOOP("ParameterDisplay", "Far X") };
    const std::array<const char *, MicGroupCount> yLabels { QT_TRANSLATE_NOOP("ParameterDisplay", "Close Y"), QT_TRANSLATE_NOOP("ParameterDisplay", "Room Y"), QT_TRANSLATE_NOOP("ParameterDisplay", "Far Y") };
    const std::array<const char *, MicGroupCount> spacingLabels { "", QT_TRANSLATE_NOOP("ParameterDisplay", "Room Spacing"), QT_TRANSLATE_NOOP("ParameterDisplay", "Far Spacing") };
    const std::array<const char *, MicGroupCount> angleLabels { "", QT_TRANSLATE_NOOP("ParameterDisplay", "Room Angle"), QT_TRANSLATE_NOOP("ParameterDisplay", "Far Angle") };
    for (size_t g = 0; g < MicGroupCount; g++) {
        describe(C::xmlKeyMicLevel(g), { .mapping = Display::Mapping::Fader, .unit = "dB", .label = levelLabels[g] });
        if (static_cast<MicGroup>(g) == MicGroup::Close) {
            describe(C::xmlKeyMicX(g), { .mapping = Display::Mapping::Bipolar, .unit = "m", .min = -MaxCloseOffsetM, .max = MaxCloseOffsetM, .rest = Display::Rest::Centre, .label = xLabels[g] });
            describe(C::xmlKeyMicY(g), { .mapping = Display::Mapping::Bipolar, .unit = "m", .min = -MaxCloseOffsetM, .max = MaxCloseOffsetM, .rest = Display::Rest::Centre, .label = yLabels[g] });
            continue;
        }
        describe(C::xmlKeyMicX(g), { .unit = "%", .min = 0.0, .max = 100.0, .label = xLabels[g] });
        describe(C::xmlKeyMicY(g), { .unit = "%", .min = 0.0, .max = 100.0, .label = yLabels[g] });
        describe(C::xmlKeyMicSpacing(g), { .unit = "m", .min = 0.0, .max = MaxSpacingM, .label = spacingLabels[g] });
        describe(C::xmlKeyMicAngle(g), { .unit = "°", .min = 0.0, .max = MaxAngleDeg, .label = angleLabels[g] });
    }

    syncParameters();
}

std::string RoomMics::typeIdString()
{
    return "28a84e1a-e669-4e5a-8738-de1836ba8f32";
}

std::string RoomMics::type() const
{
    return Constants::RackEffectType::roomMics().toStdString();
}

std::string RoomMics::typeId() const
{
    return typeIdString();
}

double RoomMics::sizeMetres(float position, double max)
{
    return ParameterMapper::mapExponential(std::clamp(static_cast<double>(position), 0.0, 1.0), MinSizeM, max);
}

float RoomMics::sizePosition(double metres, double max)
{
    return static_cast<float>(ParameterMapper::unmapExponential(std::clamp(metres, MinSizeM, max), MinSizeM, max));
}

double RoomMics::closeOffsetMetres(float position)
{
    return (std::clamp(static_cast<double>(position), 0.0, 1.0) * 2.0 - 1.0) * MaxCloseOffsetM;
}

float RoomMics::closeOffsetPosition(double metres)
{
    return static_cast<float>(std::clamp(0.5 + 0.5 * metres / MaxCloseOffsetM, 0.0, 1.0));
}

RoomSettings RoomMics::settings() const
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & name, float fallback) {
        const auto p = parameter(key(name));
        return p ? p->get().value() : fallback;
    };
    const auto indexOf = [&valueOf](const QString & name, size_t count) {
        return static_cast<size_t>(std::clamp(static_cast<int>(std::lround(valueOf(name, 0.0f))), 0, static_cast<int>(count) - 1));
    };
    const auto fraction = [&valueOf](const QString & name) {
        return std::clamp(static_cast<double>(valueOf(name, 0.5f)), 0.0, 1.0);
    };

    RoomSettings settings;
    settings.widthM = sizeMetres(valueOf(C::xmlKeyRoomWidth(), 0.5f));
    settings.depthM = sizeMetres(valueOf(C::xmlKeyRoomDepth(), 0.5f));
    settings.heightM = sizeMetres(valueOf(C::xmlKeyRoomHeight(), 0.5f), MaxHeightM);
    settings.walls = static_cast<Material>(indexOf(C::xmlKeyWallMaterial(), MaterialCount));
    settings.floor = static_cast<Material>(indexOf(C::xmlKeyFloorMaterial(), MaterialCount));
    settings.ceiling = static_cast<Material>(indexOf(C::xmlKeyCeilingMaterial(), MaterialCount));
    settings.sourceX = fraction(C::xmlKeySourceX());
    settings.sourceY = fraction(C::xmlKeySourceY());
    settings.sourceSpreadM = fraction(C::xmlKeySpread()) * MaxSpreadM;
    for (size_t g = 0; g < MicGroupCount; g++) {
        auto && group = settings.groups[g];
        if (static_cast<MicGroup>(g) == MicGroup::Close) {
            group.x = closeOffsetMetres(valueOf(C::xmlKeyMicX(g), 0.5f));
            group.y = closeOffsetMetres(valueOf(C::xmlKeyMicY(g), 0.5f));
        } else {
            group.x = fraction(C::xmlKeyMicX(g));
            group.y = fraction(C::xmlKeyMicY(g));
            group.spacingM = fraction(C::xmlKeyMicSpacing(g)) * MaxSpacingM;
            group.angleDeg = fraction(C::xmlKeyMicAngle(g)) * MaxAngleDeg;
        }
        group.pattern = static_cast<Pattern>(indexOf(C::xmlKeyMicPattern(g), PatternCount));
        group.level = ParameterMapper::mapFader(std::clamp(static_cast<double>(valueOf(C::xmlKeyMicLevel(g), 0.0f)), 0.0, 1.0));
    }
    return settings;
}

std::map<std::string, float> RoomMics::parametersFor(const RoomSettings & settings, float mix)
{
    namespace C = Constants::NahdXml;
    std::map<std::string, float> values {
        { key(C::xmlKeyRoomWidth()), sizePosition(settings.widthM) },
        { key(C::xmlKeyRoomDepth()), sizePosition(settings.depthM) },
        { key(C::xmlKeyRoomHeight()), sizePosition(settings.heightM, MaxHeightM) },
        { key(C::xmlKeyWallMaterial()), static_cast<float>(settings.walls) },
        { key(C::xmlKeyFloorMaterial()), static_cast<float>(settings.floor) },
        { key(C::xmlKeyCeilingMaterial()), static_cast<float>(settings.ceiling) },
        { key(C::xmlKeySourceX()), static_cast<float>(settings.sourceX) },
        { key(C::xmlKeySourceY()), static_cast<float>(settings.sourceY) },
        { key(C::xmlKeySpread()), static_cast<float>(settings.sourceSpreadM / MaxSpreadM) },
        { key(C::xmlKeyMix()), mix },
    };
    for (size_t g = 0; g < MicGroupCount; g++) {
        const auto & group = settings.groups[g];
        if (static_cast<MicGroup>(g) == MicGroup::Close) {
            values[key(C::xmlKeyMicX(g))] = closeOffsetPosition(group.x);
            values[key(C::xmlKeyMicY(g))] = closeOffsetPosition(group.y);
        } else {
            values[key(C::xmlKeyMicX(g))] = static_cast<float>(group.x);
            values[key(C::xmlKeyMicY(g))] = static_cast<float>(group.y);
            values[key(C::xmlKeyMicSpacing(g))] = static_cast<float>(group.spacingM / MaxSpacingM);
            values[key(C::xmlKeyMicAngle(g))] = static_cast<float>(group.angleDeg / MaxAngleDeg);
        }
        values[key(C::xmlKeyMicPattern(g))] = static_cast<float>(group.pattern);
        values[key(C::xmlKeyMicLevel(g))] = levelPosition(group.level);
    }
    return values;
}

void RoomMics::syncParameters()
{
    m_requested = settings();
}

void RoomMics::configure()
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    m_designer.configure(rate);
    const auto maxLength = static_cast<size_t>(std::ceil(ImpulseDesigner::MaxWindowSeconds * rate)) + 4;
    m_convolver.configure(maxLength);
    m_tail.configure(rate);
    for (auto && kernel : m_kernels) {
        kernel.assign(maxLength, 0.0);
    }
    m_configuredSampleRate = rate;
    m_designReady = false;
    m_hasDesign = false;
    m_hasCombined = false;
    m_lateGain = 0.0;
}

void RoomMics::combineAndLoad()
{
    // Each mic's response at its pair's level, summed per source and output
    const size_t length = m_designer.length();
    std::array<double, MicGroupCount> gains {};
    m_lateGain = 0.0;
    for (size_t g = 0; g < MicGroupCount; g++) {
        const auto group = static_cast<MicGroup>(g);
        gains[g] = m_requested.groups[g].level * m_designer.normalization(group);
        m_lateGain += gains[g] * std::sqrt(randomEnergyEfficiency(m_designer.settings().groups[g].pattern));
    }
    for (size_t source = 0; source < ImpulseDesigner::SourceCount; source++) {
        for (size_t output = 0; output < 2; output++) {
            auto && kernel = m_kernels[RoomConvolver::kernelIndex(source, output)];
            std::fill(kernel.begin(), kernel.begin() + static_cast<std::ptrdiff_t>(length), 0.0);
            for (size_t g = 0; g < MicGroupCount; g++) {
                if (gains[g] == 0.0) {
                    continue;
                }
                const auto & response = m_designer.response(ImpulseDesigner::responseIndex(static_cast<MicGroup>(g), source, output));
                for (size_t n = 0; n < length; n++) {
                    kernel[n] += gains[g] * response[n];
                }
            }
        }
    }
    std::array<const double *, RoomConvolver::KernelCount> kernels {};
    for (size_t k = 0; k < RoomConvolver::KernelCount; k++) {
        kernels[k] = m_kernels[k].data();
    }
    m_convolver.startLoad(kernels, length);
    m_tail.set(m_designer.settings(), m_designer.reverbTimes(), m_designer.windowSeconds() - m_designer.crossoverSeconds());

    m_combined = m_designer.settings();
    for (size_t g = 0; g < MicGroupCount; g++) {
        m_combined.groups[g].level = m_requested.groups[g].level;
    }
    m_hasCombined = true;
    m_designReady = false;
}

void RoomMics::advanceJobs(size_t frames, bool complete)
{
    // Nothing to play yet is as urgent as a render: the room would be missing until it was done
    complete = complete || !m_convolver.hasKernels();

    // A finished design is loaded before the next one starts, which would write over it. The
    // levels only rescale a design, so a change of level alone is a new load and no new design.
    if (!m_convolver.isLoading() && !m_designer.isDesigning() && m_hasDesign) {
        if (m_designReady || !m_requested.sameLevels(m_combined)) {
            combineAndLoad();
        }
    }
    if (!m_designer.isDesigning() && !m_designReady && (!m_hasDesign || !m_requested.sameGeometry(m_designer.settings()))) {
        m_designer.start(m_requested);
    }

    if (m_designer.isDesigning()) {
        bool finished = false;
        if (complete) {
            m_designer.finish();
            finished = true;
        } else {
            finished = m_designer.advance(static_cast<double>(frames) * designWorkPerFrame);
        }
        if (finished) {
            m_hasDesign = true;
            m_designReady = true;
            if (!m_convolver.isLoading()) {
                combineAndLoad();
            }
        }
    }

    if (complete) {
        m_convolver.finishLoad();
    } else {
        m_convolver.advanceLoad(static_cast<size_t>(std::ceil(static_cast<double>(frames) * loadPartitionsPerFrame)));
    }
}

void RoomMics::renderFrame(double & left, double & right)
{
    double lateLeft = 0.0;
    double lateRight = 0.0;
    m_tail.process(left, right, lateLeft, lateRight);
    m_convolver.process(left, right);
    left += lateLeft * m_lateGain;
    right += lateRight * m_lateGain;
}

void RoomMics::processSample(double & left, double & right)
{
    if (m_configuredSampleRate != (m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate())) {
        configure();
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    if (m_convolver.isLoading() || m_designer.isDesigning() || !m_convolver.hasKernels() || !m_requested.sameGeometry(m_designer.settings()) || !m_requested.sameLevels(m_combined)) {
        advanceJobs(1, false);
    }
    renderFrame(left, right);
}

void RoomMics::processBlock(AudioContext & context)
{
    if (m_configuredSampleRate != (m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate())) {
        configure();
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    advanceJobs(context.frameCount, context.offline);
    for (uint32_t i = 0; i < context.frameCount; i++) {
        renderFrame(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void RoomMics::finishDesign()
{
    if (m_configuredSampleRate != (m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate())) {
        configure();
    }
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    // Twice: a level change waiting on the first load is loaded by the second
    for (int i = 0; i < 2; i++) {
        advanceJobs(0, true);
        // The new responses go live at a block boundary: run silence up to it
        for (size_t n = 0; n < RoomConvolver::BlockSize && m_convolver.isLoading(); n++) {
            double left = 0.0;
            double right = 0.0;
            m_convolver.process(left, right);
        }
    }
}

void RoomMics::reset()
{
    m_convolver.reset();
    m_tail.reset();
}

void RoomMics::sync()
{
    m_shouldSyncParameters = true;
}

bool RoomMics::isSettled() const
{
    return !m_convolver.isRinging() && !m_tail.isRinging();
}

const EffectPresetList & RoomMics::factoryPresets() const
{
    using G = RoomSettings::Group;
    const auto off = 0.0;
    const auto db = [](double value) {
        return std::pow(10.0, value / 20.0);
    };
    const auto room = [](double width, double depth, double height, Material walls, Material floor, Material ceiling, double sourceX, double sourceY, double spread, G close, G roomPair, G far) {
        RoomSettings settings;
        settings.widthM = width;
        settings.depthM = depth;
        settings.heightM = height;
        settings.walls = walls;
        settings.floor = floor;
        settings.ceiling = ceiling;
        settings.sourceX = sourceX;
        settings.sourceY = sourceY;
        settings.sourceSpreadM = spread;
        settings.groups = { close, roomPair, far };
        return settings;
    };
    const auto close = [](Pattern pattern, double level) {
        return G { .pattern = pattern, .level = level };
    };
    const auto pair = [](double x, double y, double spacing, double angle, Pattern pattern, double level) {
        return G { .x = x, .y = y, .spacingM = spacing, .angleDeg = angle, .pattern = pattern, .level = level };
    };
    using M = Material;
    using P = Pattern;
    // Init first, as the way back; the rest from the smallest room to the largest
    static const EffectPresetList presets {
        { "Init", {} },
        // Dead and tight: the room only as a little air around the source
        { "Vocal Booth", parametersFor(room(2.4, 2.0, 2.4, M::Curtain, M::Carpet, M::AcousticPanel, 0.5, 0.35, 0.3, close(P::Cardioid, 1.0), pair(0.5, 0.75, 0.4, 0.0, P::Omni, db(-12.0)), pair(0.5, 0.92, 1.0, 0.0, P::Omni, off)), 1.0f) },
        // Small, hard and bright: tiles taken as brick, which they absorb about as little as
        { "Bathroom", parametersFor(room(2.5, 2.2, 2.5, M::Brick, M::Brick, M::Wood, 0.4, 0.35, 0.3, close(P::Cardioid, 1.0), pair(0.6, 0.75, 0.5, 0.0, P::Omni, db(-4.0)), pair(0.5, 0.92, 1.0, 0.0, P::Omni, off)), 1.0f) },
        // Brick around a kit: punchy, with the room pair close enough to keep the attack
        { "Drum Room", parametersFor(room(6.0, 5.0, 3.2, M::Brick, M::Wood, M::AcousticPanel, 0.5, 0.35, 1.2, close(P::Cardioid, 1.0), pair(0.5, 0.7, 1.5, 0.0, P::Omni, db(-4.0)), pair(0.5, 0.92, 3.0, 0.0, P::Omni, db(-10.0))), 1.0f) },
        // Block walls, a concrete floor and a low wooden ceiling
        { "Garage", parametersFor(room(6.0, 6.0, 2.8, M::Brick, M::Concrete, M::Wood, 0.5, 0.3, 1.0, close(P::Cardioid, 1.0), pair(0.5, 0.65, 1.0, 0.0, P::Omni, db(-6.0)), pair(0.5, 0.9, 2.5, 0.0, P::Omni, db(-12.0))), 1.0f) },
        // A medium wooden room with a treated ceiling: the default
        { "Live Room", parametersFor(defaultSettings(), 1.0f) },
        // A big wooden tracking room, room pair wide and far pair at the back
        { "Large Studio", parametersFor(room(16.0, 12.0, 6.0, M::Wood, M::Wood, M::AcousticPanel, 0.5, 0.3, 2.5, close(P::Cardioid, 1.0), pair(0.5, 0.55, 2.0, 0.0, P::Omni, db(-6.0)), pair(0.5, 0.9, 5.0, 0.0, P::Omni, db(-10.0))), 1.0f) },
        // A plastered echo chamber: a speaker at one end, the mics at the other, blended under the dry
        { "Echo Chamber", parametersFor(room(6.0, 4.0, 4.0, M::Plaster, M::Brick, M::Wood, 0.3, 0.2, 0.5, close(P::Cardioid, off), pair(0.7, 0.8, 1.5, 30.0, P::Omni, 1.0), pair(0.85, 0.9, 1.0, 0.0, P::Omni, db(-6.0))), 0.35f) },
        // An audience-filled hall: a near-coincident pair in front, an outrigger pair at the back
        { "Concert Hall", parametersFor(room(40.0, 25.0, 16.0, M::Wood, M::Audience, M::Plaster, 0.5, 0.15, 6.0, close(P::Cardioid, db(-2.0)), pair(0.5, 0.35, 2.0, 45.0, P::Cardioid, db(-4.0)), pair(0.5, 0.85, 8.0, 0.0, P::Omni, db(-6.0))), 1.0f) },
        // Stone walls, high, a congregation in the pews
        { "Church", parametersFor(room(35.0, 18.0, 18.0, M::Stone, M::Audience, M::Wood, 0.5, 0.15, 4.0, close(P::Cardioid, db(-4.0)), pair(0.5, 0.4, 3.0, 0.0, P::Omni, db(-3.0)), pair(0.5, 0.9, 6.0, 0.0, P::Omni, db(-4.0))), 1.0f) },
    };
    return presets;
}

std::vector<std::string> RoomMics::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyMicLevel(static_cast<size_t>(MicGroup::Room)).toStdString(),
        NahdXml::xmlKeyMicLevel(static_cast<size_t>(MicGroup::Far)).toStdString(),
        NahdXml::xmlKeyMix().toStdString(),
        NahdXml::xmlKeyMicLevel(static_cast<size_t>(MicGroup::Close)).toStdString(),
    };
}

} // namespace noteahead
