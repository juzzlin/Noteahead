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

#include "resonance_tamer.hpp"

#include <QtGlobal>

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/audio_context.hpp"
#include "../../../domain/dsp/svf_filter.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <string>

namespace noteahead {

namespace {

constexpr double MinFrequency { 20.0 };
constexpr double MaxFrequency { 20000.0 };
constexpr double MinQ { 1.0 };
constexpr double MaxQ { 20.0 };
constexpr double MinAttackMs { 1.0 };
constexpr double MaxAttackMs { 200.0 };
constexpr double MinReleaseMs { 10.0 };
constexpr double MaxReleaseMs { 1000.0 };
//! Two peaks are one when they are closer than this share of a bell's bandwidth.
constexpr double SeparationShareOfBandwidth { 0.5 };

int toXml(float position)
{
    return static_cast<int>(std::lround(static_cast<double>(position) * 10000.0));
}

} // namespace

double ResonanceTamer::depthDb(float position)
{
    return static_cast<double>(position) * MaxDepthDb;
}

float ResonanceTamer::depthPosition(double db)
{
    return static_cast<float>(db / MaxDepthDb);
}

double ResonanceTamer::selectivityDb(float position)
{
    return MinSelectivityDb + static_cast<double>(position) * (MaxSelectivityDb - MinSelectivityDb);
}

float ResonanceTamer::selectivityPosition(double db)
{
    return static_cast<float>((db - MinSelectivityDb) / (MaxSelectivityDb - MinSelectivityDb));
}

double ResonanceTamer::sharpnessQ(float position)
{
    return ParameterMapper::mapExponential(static_cast<double>(position), MinQ, MaxQ);
}

float ResonanceTamer::sharpnessPosition(double q)
{
    return static_cast<float>(ParameterMapper::unmapExponential(q, MinQ, MaxQ));
}

double ResonanceTamer::frequencyHz(float position)
{
    return ParameterMapper::mapLogFrequency(static_cast<double>(position), MinFrequency, MaxFrequency);
}

float ResonanceTamer::frequencyPosition(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(hz, MinFrequency, MaxFrequency));
}

float ResonanceTamer::attackPosition(double ms)
{
    return static_cast<float>(ParameterMapper::unmapExponential(ms, MinAttackMs, MaxAttackMs));
}

float ResonanceTamer::releasePosition(double ms)
{
    return static_cast<float>(ParameterMapper::unmapExponential(ms, MinReleaseMs, MaxReleaseMs));
}

ResonanceTamer::ResonanceTamer()
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    const auto sharpness = sharpnessPosition(4.0);
    const auto attack = attackPosition(10.0);
    const auto release = releasePosition(150.0);
    const auto low = frequencyPosition(100.0);
    addParameter(Parameter { C::xmlKeyDepth().toStdString(), depthPosition(6.0), 0, 2400, 600, 100 });
    addParameter(Parameter { C::xmlKeySelectivity().toStdString(), selectivityPosition(6.0), 100, 1800, 600, 100 });
    addParameter(Parameter { C::xmlKeySharpness().toStdString(), sharpness, 0, 10000, toXml(sharpness), 100 });
    addParameter(Parameter { C::xmlKeyAttack().toStdString(), attack, 0, 10000, toXml(attack), 100 });
    addParameter(Parameter { C::xmlKeyRelease().toStdString(), release, 0, 10000, toXml(release), 100 });
    addParameter(Parameter { C::xmlKeyLowFreq().toStdString(), low, 0, 10000, toXml(low), 100 });
    addParameter(Parameter { C::xmlKeyHighFreq().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addParameter(Parameter { C::xmlKeyFocus().toStdString(), 0.5f, -100, 100, 0, 1 });
    addParameter(Parameter { C::xmlKeyBandCount().toStdString(), 12.0f, BellCounts.front(), BellCounts.back(), 12, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyStereoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addMixParameter(1.0f, MixLaw::Crossfade);
    // What it takes out, on its own
    addSoloParameter();

    const auto describe = [this](const QString & key, Display display) {
        setParameterDisplay(key.toStdString(), std::move(display));
    };
    describe(C::xmlKeyDepth(), { .unit = "dB", .min = 0.0, .max = MaxDepthDb, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Depth") });
    describe(C::xmlKeySelectivity(), { .unit = "dB", .min = MinSelectivityDb, .max = MaxSelectivityDb, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Selectivity") });
    describe(C::xmlKeySharpness(), { .mapping = Display::Mapping::Exponential, .unit = "", .min = MinQ, .max = MaxQ, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Sharpness") });
    describe(C::xmlKeyAttack(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = MinAttackMs, .max = MaxAttackMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Attack") });
    describe(C::xmlKeyRelease(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = MinReleaseMs, .max = MaxReleaseMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Release") });
    describe(C::xmlKeyLowFreq(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = MinFrequency, .max = MaxFrequency, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Low") });
    describe(C::xmlKeyHighFreq(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = MinFrequency, .max = MaxFrequency, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "High") });
    describe(C::xmlKeyFocus(), { .mapping = Display::Mapping::Bipolar, .unit = "%", .min = -100.0, .max = 100.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Focus") });
}

std::string ResonanceTamer::typeIdString()
{
    return Constants::RackEffectType::resonanceTamer().toStdString();
}

std::string ResonanceTamer::type() const
{
    return typeIdString();
}

std::string ResonanceTamer::typeId() const
{
    return typeIdString();
}

ResonanceTamer::StereoMode ResonanceTamer::stereoMode() const
{
    const auto p = parameter(Constants::NahdXml::xmlKeyStereoMode().toStdString());
    return static_cast<StereoMode>(std::clamp(p ? static_cast<int>(std::lround(p->get().value())) : 0, 0, 2));
}

void ResonanceTamer::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p ? p->get().value() : 0.0f;
    };

    if (const auto mode = stereoMode(); mode != m_stereoMode) {
        m_stereoMode = mode;
        for (auto && bank : m_banks) {
            bank.reset();
        }
    }
    const size_t channels = m_stereoMode == StereoMode::Linked ? 2 : 1;
    for (auto && detector : m_detectors) {
        detector.setChannelCount(channels);
    }

    const double q = sharpnessQ(valueOf(C::xmlKeySharpness()));
    const double lowHz = frequencyHz(valueOf(C::xmlKeyLowFreq()));
    const double highHz = frequencyHz(valueOf(C::xmlKeyHighFreq()));
    // A bell's bandwidth in octaves, near enough at these Qs
    const double bandwidthOctaves = 1.0 / (q * std::numbers::ln2);
    const SpectralPeakDetector::Settings settings {
        .selectivityDb = selectivityDb(valueOf(C::xmlKeySelectivity())),
        .depthDb = depthDb(valueOf(C::xmlKeyDepth())),
        .lowHz = std::min(lowHz, highHz),
        .highHz = std::max(lowHz, highHz),
        .focus = std::clamp(static_cast<double>(valueOf(C::xmlKeyFocus())) * 2.0 - 1.0, -1.0, 1.0),
        .minSeparationOctaves = bandwidthOctaves * SeparationShareOfBandwidth,
    };
    const auto bellCount = static_cast<size_t>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyBandCount()))), BellCounts.front(), BellCounts.back()));
    for (size_t i = 0; i < m_banks.size(); i++) {
        m_detectors[i].setSettings(settings);
        auto && bank = m_banks[i];
        bank.setBellCount(bellCount);
        bank.setQ(q);
        bank.setAttackMs(ParameterMapper::mapExponential(valueOf(C::xmlKeyAttack()), MinAttackMs, MaxAttackMs));
        bank.setReleaseMs(ParameterMapper::mapExponential(valueOf(C::xmlKeyRelease()), MinReleaseMs, MaxReleaseMs));
    }
}

void ResonanceTamer::updateBuffers()
{
    const auto rate = static_cast<uint32_t>(m_sampleRate);
    if (rate != m_lastSampleRate) {
        m_lastSampleRate = rate;
        const double sampleRate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
        for (auto && detector : m_detectors) {
            detector.configure(sampleRate);
        }
        for (auto && bank : m_banks) {
            bank.setSampleRate(sampleRate);
            bank.reset();
        }
        m_shouldUpdateBuffers = true;
    }
    if (m_shouldUpdateBuffers) {
        syncParameters();
        m_shouldUpdateBuffers = false;
    }
}

void ResonanceTamer::processFrame(double & left, double & right)
{
    switch (m_stereoMode) {
    case StereoMode::Linked: {
        const std::array<double, 2> frame { left, right };
        if (m_detectors[0].push(frame.data())) {
            m_banks[0].track(m_detectors[0].peaks());
        }
        m_banks[0].advance();
        left = m_banks[0].process(0, left);
        right = m_banks[0].process(1, right);
        break;
    }
    case StereoMode::Stereo:
    case StereoMode::MidSide: {
        const bool midSide = m_stereoMode == StereoMode::MidSide;
        std::array<double, 2> paths { left, right };
        if (midSide) {
            paths = { (left + right) * 0.5, (left - right) * 0.5 };
        }
        for (size_t i = 0; i < paths.size(); i++) {
            if (m_detectors[i].push(&paths[i])) {
                m_banks[i].track(m_detectors[i].peaks());
            }
            m_banks[i].advance();
            paths[i] = m_banks[i].process(0, paths[i]);
        }
        left = midSide ? paths[0] + paths[1] : paths[0];
        right = midSide ? paths[0] - paths[1] : paths[1];
        break;
    }
    }
}

void ResonanceTamer::processSample(double & left, double & right)
{
    updateBuffers();
    processFrame(left, right);
}

void ResonanceTamer::processBlock(AudioContext & context)
{
    updateBuffers();
    m_spectrumTaps.processPre(context);
    for (uint32_t i = 0; i < context.frameCount; i++) {
        processFrame(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
    m_spectrumTaps.processPost(context);
    publish();
}

void ResonanceTamer::publish()
{
    // Never waits on the dialog: what it misses now it gets with the next block
    if (std::unique_lock<std::mutex> lock { m_publishedMutex, std::try_to_lock }; lock.owns_lock()) {
        for (size_t i = 0; i < m_banks.size(); i++) {
            m_publishedCounts[i] = m_banks[i].activeBells(m_publishedBells[i]);
        }
        m_publishedQ = m_banks[0].q();
    }
}

void ResonanceTamer::reset()
{
    for (auto && detector : m_detectors) {
        detector.reset();
    }
    for (auto && bank : m_banks) {
        bank.reset();
    }
    const std::lock_guard<std::mutex> lock { m_publishedMutex };
    m_publishedCounts = {};
}

void ResonanceTamer::sync()
{
    m_shouldUpdateBuffers = true;
}

bool ResonanceTamer::isSettled() const
{
    // Not while a spectrum is shown, or it would hang at the last block that had anything in it
    return !m_spectrumTaps.isEnabled() && m_banks[0].isSettled() && m_banks[1].isSettled();
}

SpectrumTaps & ResonanceTamer::spectrumTaps()
{
    return m_spectrumTaps;
}

std::vector<double> ResonanceTamer::reductionDbAt(std::span<const double> frequencies, size_t path) const
{
    std::array<TrackingBellBank::Bell, TrackingBellBank::MaxBells> bells {};
    size_t count = 0;
    double q = 4.0;
    {
        const std::lock_guard<std::mutex> lock { m_publishedMutex };
        if (path < m_publishedCounts.size()) {
            count = m_publishedCounts[path];
            bells = m_publishedBells[path];
        }
        q = m_publishedQ;
    }
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    std::vector<double> result(frequencies.size(), 0.0);
    SvfFilter bell;
    for (size_t b = 0; b < count; b++) {
        bell.calculateBell(std::clamp(bells[b].frequencyHz, 10.0, rate * 0.45), rate, q, bells[b].gainDb);
        for (size_t i = 0; i < frequencies.size(); i++) {
            result[i] += 20.0 * std::log10(std::max(1.0e-6, bell.magnitudeAt(frequencies[i], rate)));
        }
    }
    return result;
}

double ResonanceTamer::deepestCutDb() const
{
    const std::lock_guard<std::mutex> lock { m_publishedMutex };
    double deepest = 0.0;
    for (size_t path = 0; path < m_publishedCounts.size(); path++) {
        for (size_t b = 0; b < m_publishedCounts[path]; b++) {
            deepest = std::min(deepest, m_publishedBells[path][b].gainDb);
        }
    }
    return deepest;
}

const EffectPresetList & ResonanceTamer::factoryPresets() const
{
    namespace C = Constants::NahdXml;
    const auto key = [](const QString & name) {
        return name.toStdString();
    };
    const auto preset = [&](double lowHz, double highHz, double depth, double selectivity, double q, double attackMs, double releaseMs, double focus) {
        return std::map<std::string, float> {
            { key(C::xmlKeyLowFreq()), frequencyPosition(lowHz) },
            { key(C::xmlKeyHighFreq()), frequencyPosition(highHz) },
            { key(C::xmlKeyDepth()), depthPosition(depth) },
            { key(C::xmlKeySelectivity()), selectivityPosition(selectivity) },
            { key(C::xmlKeySharpness()), sharpnessPosition(q) },
            { key(C::xmlKeyAttack()), attackPosition(attackMs) },
            { key(C::xmlKeyRelease()), releasePosition(releaseMs) },
            { key(C::xmlKeyFocus()), static_cast<float>((focus + 1.0) * 0.5) },
        };
    };
    // Init first, as the way back; the rest alphabetical
    static const EffectPresetList presets {
        { "Init", {} },
        // The cardboard of the low mids, slowly, so that it never pumps
        { "Boxy Low Mids", preset(150.0, 800.0, 6.0, 5.0, 3.0, 20.0, 250.0, 0.0) },
        // Narrow and deep, for the ring of a tom, a snare or a small room
        { "Drum Ring", preset(80.0, 2000.0, 9.0, 6.0, 8.0, 5.0, 300.0, 0.0) },
        // The fizz of a distorted guitar, leaning to the top
        { "Guitar Fizz", preset(2500.0, 12000.0, 8.0, 5.0, 4.0, 5.0, 80.0, 0.3) },
        // A little off whatever sticks out of a whole mix, and nothing else
        { "Mix Bus Gentle", preset(100.0, 20000.0, 3.0, 8.0, 4.0, 20.0, 200.0, 0.0) },
        // The harsh band of a vocal, quickly
        { "Vocal Harshness", preset(1500.0, 10000.0, 8.0, 5.0, 5.0, 3.0, 100.0, 0.0) },
    };
    return presets;
}

std::vector<std::string> ResonanceTamer::modulationPriority() const
{
    namespace NahdXml = Constants::NahdXml;
    return {
        NahdXml::xmlKeyDepth().toStdString(),
        NahdXml::xmlKeySelectivity().toStdString(),
        NahdXml::xmlKeyLowFreq().toStdString(),
        NahdXml::xmlKeyHighFreq().toStdString()
    };
}

} // namespace noteahead
