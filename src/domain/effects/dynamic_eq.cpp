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

#include "dynamic_eq.hpp"

#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

namespace noteahead {

namespace {

constexpr double MinFrequency { 20.0 };
constexpr double MaxFrequency { 20000.0 };
constexpr double MinQ { 0.1 };
constexpr double MaxQ { 10.0 };
constexpr double MinAttackMs { 0.1 };
constexpr double MaxAttackMs { 500.0 };
constexpr double MinReleaseMs { 1.0 };
constexpr double MaxReleaseMs { 2000.0 };

//! How often the band's coefficients follow the detector. The detector itself runs every frame, so
//! this only decides how finely the moving gain is stepped, and a step of 16 frames is far finer
//! than any attack the knob reaches.
constexpr uint32_t UpdateInterval { 16 };
//! The gain history: this many points, one every this many milliseconds.
constexpr size_t HistoryLength { 256 };
constexpr double HistoryIntervalMs { 10.0 };
//! How fast the detector meter falls back: a time constant, about 9 dB in this long.
constexpr double DetectorMeterFallSeconds { 0.3 };

int toXml(float position)
{
    return static_cast<int>(std::lround(static_cast<double>(position) * 10000.0));
}

float attackPosition(double ms)
{
    return static_cast<float>(ParameterMapper::unmapExponential(ms, MinAttackMs, MaxAttackMs));
}

float releasePosition(double ms)
{
    return static_cast<float>(ParameterMapper::unmapExponential(ms, MinReleaseMs, MaxReleaseMs));
}

float rangePosition(double db)
{
    return static_cast<float>((db / DynamicEq::MaxRangeDb + 1.0) * 0.5);
}

float thresholdPosition(double db)
{
    return static_cast<float>((db + 60.0) / 60.0);
}

float ratioPosition(double ratio)
{
    return static_cast<float>((ratio - 1.0) / 19.0);
}

} // namespace

double DynamicEq::frequencyHz(float position)
{
    return ParameterMapper::mapLogFrequency(static_cast<double>(position), MinFrequency, MaxFrequency);
}

float DynamicEq::frequencyPosition(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(hz, MinFrequency, MaxFrequency));
}

double DynamicEq::qValue(float position)
{
    return ParameterMapper::mapExponential(static_cast<double>(position), MinQ, MaxQ);
}

float DynamicEq::qPosition(double q)
{
    return static_cast<float>(ParameterMapper::unmapExponential(q, MinQ, MaxQ));
}

DynamicEq::DynamicEq()
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    const auto frequency = frequencyPosition(1000.0);
    const auto q = qPosition(1.4142);
    const auto attack = attackPosition(5.0);
    const auto release = releasePosition(100.0);
    addParameter(Parameter { C::xmlKeyFilterType().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyFrequency().toStdString(), frequency, 0, 10000, toXml(frequency), 100 });
    addParameter(Parameter { C::xmlKeyQ().toStdString(), q, 0, 10000, toXml(q), 100 });
    addParameter(Parameter { C::xmlKeyRange().toStdString(), rangePosition(-6.0), -2400, 2400, -600, 100 });
    // The Compressor's threshold, ratio and knee, so the two read alike
    addParameter(Parameter { C::xmlKeyThreshold().toStdString(), thresholdPosition(-20.0), -6000, 0, -2000, 100 });
    addParameter(Parameter { C::xmlKeyRatio().toStdString(), ratioPosition(4.0), 100, 2000, 400, 100 });
    addParameter(Parameter { C::xmlKeyKnee().toStdString(), 0.0f, 0, 2400, 0, 100 });
    addParameter(Parameter { C::xmlKeyAttack().toStdString(), attack, 0, 10000, toXml(attack), 100 });
    addParameter(Parameter { C::xmlKeyRelease().toStdString(), release, 0, 10000, toXml(release), 100 });
    addParameter(Parameter { C::xmlKeyDetectorSource().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyDetectorFilter().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeySideChainSourceDevice().toStdString(), -1.0f, -1, static_cast<int>(Constants::deviceRackSize()) - 1, -1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeySideChainLpf().toStdString(), 1.0f, 0, 1000, 1000, 1 });
    addParameter(Parameter { C::xmlKeyListen().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyStereoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { C::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100 });

    // The readouts the dialog shows, so that a parameter automation speaks in the same units
    const auto describe = [this](const QString & key, Display display) {
        setParameterDisplay(key.toStdString(), std::move(display));
    };
    describe(C::xmlKeyFrequency(), { .mapping = Display::Mapping::LogFrequency, .unit = "Hz", .min = MinFrequency, .max = MaxFrequency, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Frequency") });
    describe(C::xmlKeyQ(), { .mapping = Display::Mapping::Exponential, .unit = "", .min = MinQ, .max = MaxQ, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Q") });
    describe(C::xmlKeyRange(), { .unit = "dB", .min = -MaxRangeDb, .max = MaxRangeDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Gain") });
    describe(C::xmlKeyThreshold(), { .unit = "dB", .min = -60.0, .max = 0.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Threshold") });
    describe(C::xmlKeyRatio(), { .unit = ":1", .min = 1.0, .max = 20.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Ratio") });
    describe(C::xmlKeyKnee(), { .unit = "dB", .min = 0.0, .max = 24.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Knee") });
    describe(C::xmlKeyAttack(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = MinAttackMs, .max = MaxAttackMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Attack") });
    describe(C::xmlKeyRelease(), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = MinReleaseMs, .max = MaxReleaseMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Release") });
    describe(C::xmlKeyGain(), { .unit = "dB", .min = -MaxOutputGainDb, .max = MaxOutputGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Output Gain") });

    m_history.assign(HistoryLength, 0.0f);
    m_publishedHistory.assign(HistoryLength, 0.0f);
}

std::string DynamicEq::typeIdString()
{
    return Constants::RackEffectType::dynamicEq().toStdString();
}

std::string DynamicEq::type() const
{
    return typeIdString();
}

std::string DynamicEq::typeId() const
{
    return typeIdString();
}

std::optional<size_t> DynamicEq::sidechainSourceDeviceIndex() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        if (const auto index = p->get().xmlValue(); index >= 0) {
            return static_cast<size_t>(index);
        }
    }
    return {};
}

double DynamicEq::rangeDb() const
{
    const auto p = parameter(Constants::NahdXml::xmlKeyRange().toStdString());
    return p ? (static_cast<double>(p->get().value()) * 2.0 - 1.0) * MaxRangeDb : 0.0;
}

void DynamicEq::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p ? p->get().value() : 0.0f;
    };
    m_bandType = static_cast<BandType>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyFilterType()))), 0, 3));
    m_frequency = frequencyHz(valueOf(C::xmlKeyFrequency()));
    m_q = qValue(valueOf(C::xmlKeyQ()));
    m_rangeDb = rangeDb();
    m_detectorSource = valueOf(C::xmlKeyDetectorSource()) > 0.5f ? DetectorSource::External : DetectorSource::Self;
    m_detectorFilter = valueOf(C::xmlKeyDetectorFilter()) > 0.5f ? DetectorFilter::Wide : DetectorFilter::Band;
    m_stereoMode = static_cast<StereoMode>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyStereoMode()))), 0, 2));
    m_listen = valueOf(C::xmlKeyListen()) > 0.5f;
    m_outputGain = std::pow(10.0, (static_cast<double>(valueOf(C::xmlKeyGain())) * 2.0 - 1.0) * MaxOutputGainDb / 20.0);
    m_sidechainSourceDevice = sidechainSourceDeviceIndex();

    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    m_core.setSampleRate(rate);
    m_core.setThresholdDb(-60.0 + static_cast<double>(valueOf(C::xmlKeyThreshold())) * 60.0);
    m_core.setRatio(1.0 + static_cast<double>(valueOf(C::xmlKeyRatio())) * 19.0);
    m_core.setKneeDb(static_cast<double>(valueOf(C::xmlKeyKnee())) * 24.0);
    m_core.setAttackMs(ParameterMapper::mapExponential(valueOf(C::xmlKeyAttack()), MinAttackMs, MaxAttackMs));
    m_core.setReleaseMs(ParameterMapper::mapExponential(valueOf(C::xmlKeyRelease()), MinReleaseMs, MaxReleaseMs));
    for (auto && filter : m_detectorBandPass) {
        filter.calculateBandPass(m_frequency, rate, m_q);
    }
    m_detectorLpfCutoff = valueOf(C::xmlKeySideChainLpf());
    for (auto && filter : m_detectorLpf) {
        filter.setMode(CascadedSvf::Mode::LowPass);
        filter.setSampleRate(rate);
        filter.setCutoff(m_detectorLpfCutoff);
    }
    updateBand(m_appliedGainDb);
}

void DynamicEq::updateBand(double gainDb)
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    for (auto && band : m_band) {
        switch (m_bandType) {
        case BandType::Bell:
            band.calculateBell(m_frequency, rate, m_q, gainDb);
            break;
        case BandType::LowShelf:
            band.calculateLowShelf(m_frequency, rate, m_q, gainDb);
            break;
        case BandType::HighShelf:
            band.calculateHighShelf(m_frequency, rate, m_q, gainDb);
            break;
        case BandType::Flat:
            band.setBypass();
            break;
        }
    }
    m_appliedGainDb = gainDb;
}

void DynamicEq::updateBuffers()
{
    const auto rate = static_cast<uint32_t>(m_sampleRate);
    if (rate != m_lastSampleRate) {
        m_lastSampleRate = rate;
        m_historyInterval = std::max<uint32_t>(1, static_cast<uint32_t>(HistoryIntervalMs * m_sampleRate / 1000.0));
        m_detectorPeakDecay = std::exp(-1.0 / (DetectorMeterFallSeconds * std::max(1.0, m_sampleRate)));
        m_shouldUpdateBuffers = true;
    }
    if (m_shouldUpdateBuffers) {
        syncParameters();
        m_shouldUpdateBuffers = false;
    }
}

void DynamicEq::processFrame(double & left, double & right, const double * external)
{
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5;

    // What the detector hears
    double detectorL = left;
    double detectorR = right;
    if (m_detectorSource == DetectorSource::External && external) {
        detectorL = external[0];
        detectorR = external[1];
    } else if (m_stereoMode == StereoMode::Mid) {
        detectorL = detectorR = mid;
    } else if (m_stereoMode == StereoMode::Side) {
        detectorL = detectorR = side;
    }
    if (m_detectorLpfCutoff < 1.0f) {
        detectorL = m_detectorLpf[0].process(detectorL);
        detectorR = m_detectorLpf[1].process(detectorR);
    }
    if (m_detectorFilter == DetectorFilter::Band) {
        detectorL = m_detectorBandPass[0].process(detectorL);
        detectorR = m_detectorBandPass[1].process(detectorR);
    }

    m_detectorPeak = std::max(std::max(std::abs(detectorL), std::abs(detectorR)), m_detectorPeak * m_detectorPeakDecay);

    // As far as a compressor would pull the level down, towards Gain and never past it
    const double reduction = -m_core.processGainDb(detectorL, detectorR);
    const double amount = std::min(reduction, std::abs(m_rangeDb));
    m_gainDb = m_rangeDb < 0.0 ? -amount : amount;
    if (++m_framesSinceUpdate >= UpdateInterval) {
        m_framesSinceUpdate = 0;
        if (m_bandType != BandType::Flat && std::abs(m_gainDb - m_appliedGainDb) > 0.001) {
            updateBand(m_gainDb);
        }
    }

    if (m_listen) {
        // The detector instead of the output, to find what the band should be listening for
        left = detectorL;
        right = detectorR;
        return;
    }

    const auto shape = [this](size_t path, double sample) {
        return m_bandType == BandType::Flat ? sample * std::pow(10.0, m_gainDb / 20.0) : m_band.at(path).process(sample);
    };
    switch (m_stereoMode) {
    case StereoMode::Stereo:
        left = shape(0, left);
        right = shape(1, right);
        break;
    case StereoMode::Mid: {
        const double shaped = shape(0, mid);
        left = shaped + side;
        right = shaped - side;
        break;
    }
    case StereoMode::Side: {
        const double shaped = shape(1, side);
        left = mid + shaped;
        right = mid - shaped;
        break;
    }
    }
    left *= m_outputGain;
    right *= m_outputGain;
}

void DynamicEq::processSample(double & left, double & right)
{
    updateBuffers();
    processFrame(left, right, nullptr);
    m_publishedGainDb.store(static_cast<float>(m_gainDb), std::memory_order_relaxed);
    m_publishedDetectorDb.store(static_cast<float>(20.0 * std::log10(std::max(m_detectorPeak, 1.0e-6))), std::memory_order_relaxed);
}

void DynamicEq::processBlock(AudioContext & context)
{
    updateBuffers();
    const bool hasExternal = m_sidechainSourceDevice && *m_sidechainSourceDevice < context.deviceOutputBuffers.size() && context.deviceOutputBuffers[*m_sidechainSourceDevice].size() >= static_cast<size_t>(context.frameCount) * 2;
    const auto external = hasExternal ? context.deviceOutputBuffers[*m_sidechainSourceDevice] : std::span<const double> {};
    const bool analysing = m_analysisEnabled.load(std::memory_order_relaxed);
    m_spectrumTaps.processPre(context);
    m_spectrumTaps.processPost(external, context.frameCount, context.sampleRate);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processFrame(context.buffer[i * 2], context.buffer[i * 2 + 1], hasExternal ? external.data() + i * 2 : nullptr);
        if (analysing && (m_historyCountdown == 0 || --m_historyCountdown == 0)) {
            m_historyCountdown = m_historyInterval;
            m_history[m_historyIndex] = static_cast<float>(m_gainDb);
            m_historyIndex = (m_historyIndex + 1) % m_history.size();
        }
    }
    m_publishedGainDb.store(static_cast<float>(m_gainDb), std::memory_order_relaxed);
    m_publishedDetectorDb.store(static_cast<float>(20.0 * std::log10(std::max(m_detectorPeak, 1.0e-6))), std::memory_order_relaxed);

    if (analysing) {
        // Never waits on the dialog: a history it misses is replaced by the next block's
        if (std::unique_lock<std::mutex> lock { m_historyMutex, std::try_to_lock }; lock.owns_lock()) {
            for (size_t i = 0; i < m_history.size(); i++) {
                m_publishedHistory[i] = m_history[(m_historyIndex + i) % m_history.size()];
            }
        }
    }
}

void DynamicEq::reset()
{
    m_core.reset();
    for (auto && filter : m_band) {
        filter.reset();
    }
    for (auto && filter : m_detectorBandPass) {
        filter.reset();
    }
    for (auto && filter : m_detectorLpf) {
        filter.reset();
    }
    m_gainDb = 0.0;
    m_detectorPeak = 0.0;
    m_publishedDetectorDb.store(-120.0f, std::memory_order_relaxed);
    m_framesSinceUpdate = 0;
    updateBand(0.0);
    m_publishedGainDb.store(0.0f, std::memory_order_relaxed);
}

void DynamicEq::sync()
{
    m_shouldUpdateBuffers = true;
}

bool DynamicEq::isSettled() const
{
    // Not while a spectrum is shown, or it would hang at the last block that had anything in it
    return !m_spectrumTaps.isEnabled() && std::abs(m_core.reductionDb()) < 0.001;
}

SpectrumTaps & DynamicEq::spectrumTaps()
{
    return m_spectrumTaps;
}

float DynamicEq::currentGainDb() const
{
    return m_publishedGainDb.load(std::memory_order_relaxed);
}

float DynamicEq::detectorLevelDb() const
{
    return m_publishedDetectorDb.load(std::memory_order_relaxed);
}

std::vector<float> DynamicEq::gainHistory() const
{
    const std::lock_guard<std::mutex> lock { m_historyMutex };
    return m_publishedHistory;
}

void DynamicEq::setAnalysisEnabled(bool enabled)
{
    m_analysisEnabled.store(enabled, std::memory_order_relaxed);
}

std::vector<double> DynamicEq::magnitudesDbAt(std::span<const double> frequencies, double gainDb) const
{
    // Built from the parameters on a band of its own, as the equalizers draw theirs: the processing
    // one belongs to the audio thread
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p ? p->get().value() : 0.0f;
    };
    const auto type = static_cast<BandType>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyFilterType()))), 0, 3));
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    const double frequency = frequencyHz(valueOf(C::xmlKeyFrequency()));
    const double q = qValue(valueOf(C::xmlKeyQ()));
    const double outputDb = (static_cast<double>(valueOf(C::xmlKeyGain())) * 2.0 - 1.0) * MaxOutputGainDb;
    SvfFilter band;
    switch (type) {
    case BandType::Bell:
        band.calculateBell(frequency, rate, q, gainDb);
        break;
    case BandType::LowShelf:
        band.calculateLowShelf(frequency, rate, q, gainDb);
        break;
    case BandType::HighShelf:
        band.calculateHighShelf(frequency, rate, q, gainDb);
        break;
    case BandType::Flat:
        band.setBypass();
        break;
    }
    std::vector<double> result;
    result.reserve(frequencies.size());
    for (const double hz : frequencies) {
        const double db = type == BandType::Flat ? gainDb : 20.0 * std::log10(std::max(1.0e-6, band.magnitudeAt(hz, rate)));
        result.push_back(db + outputDb);
    }
    return result;
}

const EffectPresetList & DynamicEq::factoryPresets() const
{
    namespace C = Constants::NahdXml;
    const auto key = [](const QString & name) {
        return name.toStdString();
    };
    const auto band = [&](BandType type, double hz, double q, double rangeDb, double thresholdDb, double ratio, double attackMs, double releaseMs) {
        return std::map<std::string, float> {
            { key(C::xmlKeyFilterType()), static_cast<float>(static_cast<int>(type)) },
            { key(C::xmlKeyFrequency()), frequencyPosition(hz) },
            { key(C::xmlKeyQ()), qPosition(q) },
            { key(C::xmlKeyRange()), rangePosition(rangeDb) },
            { key(C::xmlKeyThreshold()), thresholdPosition(thresholdDb) },
            { key(C::xmlKeyRatio()), ratioPosition(ratio) },
            { key(C::xmlKeyAttack()), attackPosition(attackMs) },
            { key(C::xmlKeyRelease()), releasePosition(releaseMs) },
        };
    };
    const auto keyedBy = [&](std::map<std::string, float> parameters, bool wide) {
        parameters[key(C::xmlKeyDetectorSource())] = 1.0f;
        parameters[key(C::xmlKeyDetectorFilter())] = wide ? 1.0f : 0.0f;
        return parameters;
    };
    // Init first, as the way back; the rest alphabetical. The keyed ones need a side-chain source
    // picked, which a preset cannot know.
    static const EffectPresetList presets {
        { "Init", {} },
        // Brings a snare's crack up only when it is hit hard
        { "Crack on Hits", band(BandType::Bell, 4000.0, 1.4, 5.0, -24.0, 4.0, 1.0, 80.0) },
        // The esses, only while they are there
        { "De-Ess", band(BandType::Bell, 6500.0, 2.0, -8.0, -30.0, 6.0, 1.0, 60.0) },
        // The whole signal down while the key is loud: a ducker that listens to a band of it
        { "Ducker", keyedBy(band(BandType::Flat, 100.0, 1.4, -12.0, -30.0, 8.0, 5.0, 200.0), true) },
        // The bass's low end out of the way of the kick, and nothing else of the bass touched
        { "Kick Ducks Bass", keyedBy(band(BandType::Bell, 60.0, 1.4, -8.0, -30.0, 6.0, 2.0, 120.0), false) },
        // A ringing note that only rings when it is played loud
        { "Tame Resonance", band(BandType::Bell, 300.0, 4.0, -6.0, -24.0, 4.0, 5.0, 120.0) },
    };
    return presets;
}

} // namespace noteahead
