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

#include "chorus_effect.hpp"
#include "audio_context.hpp"
#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

ChorusEffect::ChorusEffect()
{
    addParameter({ Constants::NahdXml::xmlKeyRate().toStdString(), 0.25f, 0, 1000, 250, 1, Parameter::Type::Continuous, { "chorusRate" } });
    addParameter({ Constants::NahdXml::xmlKeyDepth().toStdString(), 0.5f, 0, 1000, 500, 1, Parameter::Type::Continuous, { "chorusDepth" } });
    addParameter({ Constants::NahdXml::xmlKeyDelay().toStdString(), 0.4f, 0, 1000, 400, 1, Parameter::Type::Continuous, { "chorusDelay" } });
    addParameter({ Constants::NahdXml::xmlKeyWidth().toStdString(), 1.0f, 0, 1000, 1000, 1, Parameter::Type::Continuous, { "chorusWidth" } });
    addParameter({ Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 1.0f, 0, 1000, 1000, 1, Parameter::Type::Continuous, { "chorusLpf" } });
    addParameter({ Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 1000, 0, 1, Parameter::Type::Continuous, { "chorusHpf" } });
    addParameter({ Constants::NahdXml::xmlKeyMix().toStdString(), 0.5f, 0, 1000, 500, 1, Parameter::Type::Continuous, { "chorusMix" } });

    m_lfoL.setWaveform(Lfo::Waveform::Sine);
    m_lfoR.setWaveform(Lfo::Waveform::Sine);
    m_lfoR.setPhase(0.25); // 90 degree offset

    m_hpfL.setMode(CascadedSvf::Mode::HighPass);
    m_hpfR.setMode(CascadedSvf::Mode::HighPass);
    m_lpfL.setMode(CascadedSvf::Mode::LowPass);
    m_lpfR.setMode(CascadedSvf::Mode::LowPass);

    m_hpfL.setOrder(2);
    m_hpfR.setOrder(2);
    m_lpfL.setOrder(2);
    m_lpfR.setOrder(2);

    ChorusEffect::syncParameters();
}

void ChorusEffect::process(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldUpdateBuffers) {
        syncParameters();
        updateBuffers();
        m_shouldUpdateBuffers = false;
        m_shouldSyncParameters = false;
    } else if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }

    const double dryL = left;
    const double dryR = right;

    // Input HPF
    double filteredL = m_hpfL.process(dryL);
    double filteredR = m_hpfR.process(dryR);

    // Write to buffers
    m_bufferL[m_writePos] = filteredL;
    m_bufferR[m_writePos] = filteredR;

    // LFO modulation
    const double modL = m_lfoL.nextSample(); // -1.0 to 1.0
    const double modR = m_lfoR.nextSample(); // -1.0 to 1.0

    // Calculate delay in samples
    // baseDelay + mod * depth
    // Depth 1.0 means +/- 4ms modulation (musical for chorus)
    const double baseDelaySamples = m_delayMs * 0.001 * m_sampleRate;
    const double depthSamples = m_depth * 0.004 * m_sampleRate;

    auto readFromBuffer = [&](const std::vector<double> & buffer, double delaySamples) {
        delaySamples = std::max(1.0, delaySamples);
        const double bufSize = static_cast<double>(buffer.size());
        double readPos = static_cast<double>(m_writePos) - delaySamples;
        while (readPos < 0.0)
            readPos += bufSize;
        while (readPos >= bufSize)
            readPos -= bufSize;

        const size_t i0 = static_cast<size_t>(readPos);
        const size_t i1 = (i0 + 1) % buffer.size();
        const double frac = readPos - static_cast<double>(i0);

        return buffer[i0] * (1.0 - frac) + buffer[i1] * frac;
    };

    // Dual-tap modulation for thicker sound
    double wetL = (readFromBuffer(m_bufferL, baseDelaySamples + modL * depthSamples) + readFromBuffer(m_bufferL, baseDelaySamples - modL * depthSamples)) * 0.5;
    double wetR = (readFromBuffer(m_bufferR, baseDelaySamples + modR * depthSamples) + readFromBuffer(m_bufferR, baseDelaySamples - modR * depthSamples)) * 0.5;

    // Stereo Width (Mid-Side approach)
    const double mid = (wetL + wetR) * 0.5;
    const double side = (wetL - wetR) * 0.5;
    wetL = mid + side * m_width;
    wetR = mid - side * m_width;

    // Output LPF
    wetL = m_lpfL.process(wetL);
    wetR = m_lpfR.process(wetR);

    // Constant-power crossfade keeps the send-mode delta (output - dry) non-negative at mix <= 0.5.
    const double dryCoeff = std::clamp(2.0 * (1.0 - m_mix), 0.0, 1.0);
    const double wetCoeff = std::clamp(2.0 * m_mix, 0.0, 1.0);
    left = dryL * dryCoeff + wetL * wetCoeff;
    right = dryR * dryCoeff + wetR * wetCoeff;

    // Update write position
    m_writePos = (m_writePos + 1) % static_cast<uint32_t>(m_bufferL.size());
}

void ChorusEffect::reset()
{
    std::fill(m_bufferL.begin(), m_bufferL.end(), 0.0);
    std::fill(m_bufferR.begin(), m_bufferR.end(), 0.0);
    m_writePos = 0;
    m_lfoL.reset();
    m_lfoR.reset();
    m_lfoR.setPhase(0.25);
    m_hpfL.reset();
    m_hpfR.reset();
    m_lpfL.reset();
    m_lpfR.reset();
}

void ChorusEffect::sync()
{
    m_shouldSyncParameters = true;
}

std::string ChorusEffect::typeIdString()
{
    return Constants::RackEffectType::chorus().toStdString();
}

std::string ChorusEffect::type() const
{
    return Constants::RackEffectType::chorus().toStdString();
}

std::string ChorusEffect::typeId() const
{
    return typeIdString();
}

void ChorusEffect::setRate(double rate)
{
    parameter(Constants::NahdXml::xmlKeyRate().toStdString())->get().update(static_cast<float>(rate));
    m_shouldSyncParameters = true;
}

double ChorusEffect::rate() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRate().toStdString()); p) {
        return p->get().value();
    }
    return 0.25;
}

void ChorusEffect::setDepth(double depth)
{
    parameter(Constants::NahdXml::xmlKeyDepth().toStdString())->get().update(static_cast<float>(depth));
    m_shouldSyncParameters = true;
}

double ChorusEffect::depth() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDepth().toStdString()); p) {
        return p->get().value();
    }
    return 0.5;
}

void ChorusEffect::setDelay(double ms)
{
    parameter(Constants::NahdXml::xmlKeyDelay().toStdString())->get().update(static_cast<float>(ms));
    m_shouldSyncParameters = true;
    m_shouldUpdateBuffers = true;
}

double ChorusEffect::delay() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelay().toStdString()); p) {
        return p->get().value();
    }
    return 0.4;
}

void ChorusEffect::setMix(double mix)
{
    parameter(Constants::NahdXml::xmlKeyMix().toStdString())->get().update(static_cast<float>(mix));
    m_shouldSyncParameters = true;
}

double ChorusEffect::mix() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        return p->get().value();
    }
    return 0.5;
}

void ChorusEffect::setWidth(double width)
{
    parameter(Constants::NahdXml::xmlKeyWidth().toStdString())->get().update(static_cast<float>(width));
    m_shouldSyncParameters = true;
}

double ChorusEffect::width() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyWidth().toStdString()); p) {
        return p->get().value();
    }
    return 1.0;
}

void ChorusEffect::setLpfCutoff(double cutoff)
{
    parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString())->get().update(static_cast<float>(cutoff));
    m_shouldSyncParameters = true;
}

double ChorusEffect::lpfCutoff() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        return p->get().value();
    }
    return 1.0;
}

void ChorusEffect::setHpfCutoff(double cutoff)
{
    parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString())->get().update(static_cast<float>(cutoff));
    m_shouldSyncParameters = true;
}

double ChorusEffect::hpfCutoff() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        return p->get().value();
    }
    return 0.0;
}

void ChorusEffect::syncParameters()
{
    const auto rateParam = parameter(Constants::NahdXml::xmlKeyRate().toStdString());
    const auto depthParam = parameter(Constants::NahdXml::xmlKeyDepth().toStdString());
    const auto delayParam = parameter(Constants::NahdXml::xmlKeyDelay().toStdString());
    const auto widthParam = parameter(Constants::NahdXml::xmlKeyWidth().toStdString());
    const auto lpfParam = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString());
    const auto hpfParam = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString());
    const auto mixParam = parameter(Constants::NahdXml::xmlKeyMix().toStdString());

    m_rate = ParameterMapper::mapExponential(rateParam->get().value(), 0.1, 10.0);
    m_depth = depthParam->get().value();
    m_delayMs = 5.0 + delayParam->get().value() * 25.0; // 5.0 to 30.0 ms
    m_width = widthParam->get().value();
    m_lpfCutoff = lpfParam->get().value();
    m_hpfCutoff = hpfParam->get().value();
    m_mix = mixParam->get().value();

    m_lfoL.setSampleRate(m_sampleRate);
    m_lfoR.setSampleRate(m_sampleRate);
    m_lfoL.setFrequency(m_rate);
    m_lfoR.setFrequency(m_rate);

    updateFilters();
}

void ChorusEffect::updateBuffers()
{
    m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
    // Max delay (base + mod) is around 50ms + 10ms = 60ms.
    // Let's allocate 100ms to be safe.
    const uint32_t size = static_cast<uint32_t>(0.1 * m_sampleRate);
    if (m_bufferL.size() != size) {
        m_bufferL.assign(size, 0.0);
        m_bufferR.assign(size, 0.0);
        m_writePos = 0;
    }
}

void ChorusEffect::updateFilters()
{
    m_hpfL.setSampleRate(m_sampleRate);
    m_hpfR.setSampleRate(m_sampleRate);
    m_lpfL.setSampleRate(m_sampleRate);
    m_lpfR.setSampleRate(m_sampleRate);

    m_hpfL.setCutoff(m_hpfCutoff);
    m_hpfR.setCutoff(m_hpfCutoff);
    m_lpfL.setCutoff(m_lpfCutoff);
    m_lpfR.setCutoff(m_lpfCutoff);
}

} // namespace noteahead
