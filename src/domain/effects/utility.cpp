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

#include "utility.hpp"

#include <QtGlobal>

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace noteahead {

namespace {

constexpr double MinMonoBassHz { 20.0 };
constexpr double MaxMonoBassHz { 500.0 };
constexpr int DefaultMonoBassHz { 120 };

//! How long the readings average over: long enough to read, short enough to follow a change.
constexpr double ReadingSeconds { 0.3 };
//! A hit is the reference rising this far above where it has been, from at least this level.
constexpr double OnsetRatio { 2.5 };
constexpr double OnsetFloor { 0.01 };
//! What is captured from each hit, and how long after one the next can be.
constexpr double CaptureSeconds { 0.04 };
constexpr double HoldOffSeconds { 0.15 };

constexpr double SilenceFloor { 1.0e-10 };

double toDb(double meanSquare)
{
    return 10.0 * std::log10(std::max(meanSquare, SilenceFloor));
}

} // namespace

Utility::Utility()
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    addParameter(Parameter { C::xmlKeyGain().toStdString(), 0.5f, -2400, 2400, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyInvertLeft().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyInvertRight().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyChannelMode().toStdString(), 0.0f, 0, 4, 0, 1, Parameter::Type::Discrete });
    // 100 %, which leaves the image as it is
    addParameter(Parameter { C::xmlKeyWidth().toStdString(), 0.5f, 0, 20000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyMonoBass().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyMonoBassFrequency().toStdString(), Parameter::xmlValueToInternal(DefaultMonoBassHz, static_cast<int>(MinMonoBassHz), static_cast<int>(MaxMonoBassHz)), static_cast<int>(MinMonoBassHz), static_cast<int>(MaxMonoBassHz), DefaultMonoBassHz, 1, Parameter::Type::Continuous });
    // A thousandth of a millisecond, which is finer than a sample even at 192 kHz
    addParameter(Parameter { C::xmlKeyDelayTime().toStdString(), 0.0f, 0, 20000, 0, 1000, Parameter::Type::Continuous });
    addParameter(Parameter { C::xmlKeyDcBlock().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeySideChainSourceDevice().toStdString(), -1.0f, -1, static_cast<int>(Constants::deviceRackSize()) - 1, -1, 1, Parameter::Type::Discrete });

    // The readouts the dialog shows, so that a parameter automation speaks in the same units
    const auto describe = [this](const QString & key, Display display) {
        setParameterDisplay(key.toStdString(), std::move(display));
    };
    describe(C::xmlKeyGain(), { .unit = "dB", .min = -MaxGainDb, .max = MaxGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Gain") });
    describe(C::xmlKeyWidth(), { .mapping = Display::Mapping::Value, .unit = "%", .min = 0.0, .max = 200.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Width") });
    describe(C::xmlKeyMonoBassFrequency(), { .unit = "Hz", .min = MinMonoBassHz, .max = MaxMonoBassHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Bass Mono Frequency") });
    describe(C::xmlKeyDelayTime(), { .unit = "ms", .min = 0.0, .max = MaxDelayMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Delay") });
}

std::string Utility::typeIdString()
{
    return Constants::RackEffectType::utility().toStdString();
}

std::string Utility::type() const
{
    return typeIdString();
}

std::string Utility::typeId() const
{
    return typeIdString();
}

uint32_t Utility::delaySamples(float parameterValue, double sampleRate)
{
    const double ms = static_cast<double>(std::clamp(parameterValue, 0.0f, 1.0f)) * MaxDelayMs;
    return static_cast<uint32_t>(std::lround(ms * sampleRate / 1000.0));
}

std::optional<size_t> Utility::sidechainSourceDeviceIndex() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        if (const auto index = p->get().xmlValue(); index >= 0) {
            return static_cast<size_t>(index);
        }
    }
    return {};
}

void Utility::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & key) {
        const auto p = parameter(key.toStdString());
        return p ? p->get().value() : 0.0f;
    };
    m_gain = std::pow(10.0, (static_cast<double>(valueOf(C::xmlKeyGain())) * 2.0 - 1.0) * MaxGainDb / 20.0);
    m_invertLeft = valueOf(C::xmlKeyInvertLeft()) > 0.5f;
    m_invertRight = valueOf(C::xmlKeyInvertRight()) > 0.5f;
    m_channelMode = static_cast<ChannelMode>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyChannelMode()))), 0, 4));
    m_width = static_cast<double>(valueOf(C::xmlKeyWidth())) * 2.0;
    m_monoBass = valueOf(C::xmlKeyMonoBass()) > 0.5f;
    m_dcBlock = valueOf(C::xmlKeyDcBlock()) > 0.5f;
    m_sidechainSourceDevice = sidechainSourceDeviceIndex();

    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    m_sideSplit.setSampleRate(rate);
    m_sideSplit.setCutoff(MinMonoBassHz + static_cast<double>(valueOf(C::xmlKeyMonoBassFrequency())) * (MaxMonoBassHz - MinMonoBassHz));
    for (auto && blocker : m_dcBlockers) {
        blocker.setSampleRate(rate);
    }
    m_delaySamples = delaySamples(valueOf(C::xmlKeyDelayTime()), rate);
}

void Utility::updateBuffers()
{
    const auto rate = static_cast<uint32_t>(m_sampleRate);
    if (rate != m_lastSampleRate) {
        m_lastSampleRate = rate;
        // The longest delay at this rate and the frame being written
        const size_t length = delaySamples(1.0f, static_cast<double>(rate)) + 1;
        for (auto && line : m_delayLines) {
            line.assign(length, 0.0);
        }
        m_delayIndex = 0;
        m_smoothing = std::exp(-1.0 / (ReadingSeconds * m_sampleRate));
        m_fastRelease = std::exp(-1.0 / (0.005 * m_sampleRate));
        m_slowCoefficient = 1.0 - std::exp(-1.0 / (0.2 * m_sampleRate));
        m_captureLength = static_cast<size_t>(CaptureSeconds * m_sampleRate);
        m_holdOffLength = static_cast<size_t>(HoldOffSeconds * m_sampleRate);
        m_pending.reference.reserve(m_captureLength);
        m_pending.signal.reserve(m_captureLength);
        m_shouldUpdateBuffers = true;
    }
    if (m_shouldUpdateBuffers) {
        syncParameters();
        m_shouldUpdateBuffers = false;
    }
}

void Utility::processFrame(double & left, double & right)
{
    m_quietFrames = (left != 0.0 || right != 0.0) ? 0 : m_quietFrames + 1;

    if (m_dcBlock) {
        left = m_dcBlockers[0].process(left);
        right = m_dcBlockers[1].process(right);
    }

    switch (m_channelMode) {
    case ChannelMode::Stereo:
        break;
    case ChannelMode::Left:
        right = left;
        break;
    case ChannelMode::Right:
        left = right;
        break;
    case ChannelMode::Swap:
        std::swap(left, right);
        break;
    case ChannelMode::Mono:
        left = right = (left + right) * 0.5;
        break;
    }

    if (m_invertLeft) {
        left = -left;
    }
    if (m_invertRight) {
        right = -right;
    }

    if (m_width != 1.0 || m_monoBass) {
        const double mid = (left + right) * 0.5;
        double side = (left - right) * 0.5 * m_width;
        if (m_monoBass) {
            // The side's low band dropped: below the corner both channels carry the same signal
            double low = 0.0;
            double high = 0.0;
            m_sideSplit.process(side, low, high);
            side = high;
        }
        left = mid + side;
        right = mid - side;
    }

    if (m_delaySamples > 0 && !m_delayLines[0].empty()) {
        const size_t length = m_delayLines[0].size();
        const size_t read = (m_delayIndex + length - std::min<size_t>(m_delaySamples, length - 1)) % length;
        m_delayLines[0][m_delayIndex] = left;
        m_delayLines[1][m_delayIndex] = right;
        left = m_delayLines[0][read];
        right = m_delayLines[1][read];
        m_delayIndex = (m_delayIndex + 1) % length;
    }

    left *= m_gain;
    right *= m_gain;
}

void Utility::processSample(double & left, double & right)
{
    updateBuffers();
    processFrame(left, right);
}

void Utility::processBlock(AudioContext & context)
{
    updateBuffers();
    const bool analysing = m_analysisEnabled.load(std::memory_order_relaxed);
    const bool hasReference = m_sidechainSourceDevice && *m_sidechainSourceDevice < context.deviceOutputBuffers.size() && !context.deviceOutputBuffers[*m_sidechainSourceDevice].empty();
    const auto reference = hasReference ? context.deviceOutputBuffers[*m_sidechainSourceDevice] : std::span<const double> {};

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double & left = context.buffer[i * 2];
        double & right = context.buffer[i * 2 + 1];
        processFrame(left, right);
        if (analysing) {
            const double referenceSample = hasReference && i * 2 + 1 < reference.size() ? (reference[i * 2] + reference[i * 2 + 1]) * 0.5 : 0.0;
            analyse(referenceSample, (left + right) * 0.5, hasReference);
        }
    }

    if (analysing) {
        Reading reading;
        reading.hasReference = hasReference;
        if (m_referenceSquare > SilenceFloor && m_signalSquare > SilenceFloor) {
            reading.correlation = static_cast<float>(std::clamp(m_productSum / std::sqrt(m_referenceSquare * m_signalSquare), -1.0, 1.0));
            reading.sumGainDb = static_cast<float>(toDb(m_sumSquare) - toDb(m_referenceSquare + m_signalSquare));
        }
        reading.referenceDb = static_cast<float>(toDb(m_referenceSquare));
        reading.signalDb = static_cast<float>(toDb(m_signalSquare));
        // Never waits on the dialog: a reading it misses is replaced by the next block's
        if (std::unique_lock<std::mutex> lock { m_publishMutex, std::try_to_lock }; lock.owns_lock()) {
            m_reading = reading;
            if (m_captureReady) {
                std::swap(m_capture, m_pending);
                m_pending.reference.clear();
                m_pending.signal.clear();
                m_captureReady = false;
            }
        }
    }
}

void Utility::analyse(double referenceSample, double signalSample, bool hasReference)
{
    const double c = m_smoothing;
    m_productSum = c * m_productSum + (1.0 - c) * referenceSample * signalSample;
    m_referenceSquare = c * m_referenceSquare + (1.0 - c) * referenceSample * referenceSample;
    m_signalSquare = c * m_signalSquare + (1.0 - c) * signalSample * signalSample;
    const double sum = referenceSample + signalSample;
    m_sumSquare = c * m_sumSquare + (1.0 - c) * sum * sum;

    // Hits of the reference, or of this signal when there is none to listen to
    const double trigger = std::abs(hasReference ? referenceSample : signalSample);
    m_fastEnvelope = std::max(trigger, m_fastEnvelope * m_fastRelease);
    const bool onset = m_fastEnvelope > OnsetFloor && m_fastEnvelope > OnsetRatio * m_slowEnvelope;
    m_slowEnvelope += (m_fastEnvelope - m_slowEnvelope) * m_slowCoefficient;
    if (m_holdOff > 0) {
        m_holdOff--;
    }

    if (!m_capturing && !m_captureReady && m_holdOff == 0 && onset) {
        m_capturing = true;
        m_captured = 0;
        m_pending.reference.clear();
        m_pending.signal.clear();
        m_holdOff = m_holdOffLength;
    }
    if (m_capturing) {
        if (hasReference) {
            m_pending.reference.push_back(static_cast<float>(referenceSample));
        }
        m_pending.signal.push_back(static_cast<float>(signalSample));
        if (++m_captured >= m_captureLength) {
            m_capturing = false;
            m_captureReady = true;
        }
    }
}

void Utility::reset()
{
    for (auto && blocker : m_dcBlockers) {
        blocker.reset();
    }
    m_sideSplit.reset();
    for (auto && line : m_delayLines) {
        std::ranges::fill(line, 0.0);
    }
    m_delayIndex = 0;
    m_quietFrames = 0;
    m_productSum = m_referenceSquare = m_signalSquare = m_sumSquare = 0.0;
    m_fastEnvelope = m_slowEnvelope = 0.0;
    m_capturing = false;
}

void Utility::sync()
{
    m_shouldUpdateBuffers = true;
}

bool Utility::isSettled() const
{
    return !m_analysisEnabled.load(std::memory_order_relaxed) && m_quietFrames >= m_delaySamples;
}

void Utility::setAnalysisEnabled(bool enabled)
{
    m_analysisEnabled.store(enabled, std::memory_order_relaxed);
}

Utility::Reading Utility::reading() const
{
    const std::lock_guard<std::mutex> lock { m_publishMutex };
    return m_reading;
}

Utility::Capture Utility::capture() const
{
    const std::lock_guard<std::mutex> lock { m_publishMutex };
    return m_capture;
}

} // namespace noteahead
