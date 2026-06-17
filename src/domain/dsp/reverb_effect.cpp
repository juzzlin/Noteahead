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

#include "reverb_effect.hpp"

#include "../../common/constants.hpp"
#include "audio_context.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace noteahead {

ReverbEffect::ReverbEffect()
{
    addParameter({ Constants::NahdXml::xmlKeySize().toStdString(), 0.5f, 0, 10000, 5000, 100, Parameter::Type::Continuous, { "reverbSize" } });
    addParameter({ Constants::NahdXml::xmlKeyDecay().toStdString(), 0.15f, 0, 10000, 1500, 1, Parameter::Type::Continuous, { "reverbDecay" } });
    addParameter({ Constants::NahdXml::xmlKeyDamping().toStdString(), 0.3f, 0, 10000, 3000, 100, Parameter::Type::Continuous, { "reverbDamping" } });
    addParameter({ Constants::NahdXml::xmlKeyPreDelay().toStdString(), 0.04f, 0, 500, 20, 1, Parameter::Type::Continuous, { "reverbPreDelay" } });
    addParameter({ Constants::NahdXml::xmlKeyWidth().toStdString(), 0.5f, 0, 200, 100, 1, Parameter::Type::Continuous, { "reverbWidth" } });
    addParameter({ Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 0.85f, 0, 10000, 8500, 100, Parameter::Type::Continuous, { "reverbLpfCutoff" } });
    addParameter({ Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.2f, 0, 10000, 2000, 100, Parameter::Type::Continuous, { "reverbHpfCutoff" } });
    addParameter({ Constants::NahdXml::xmlKeyMix().toStdString(), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous, { "reverbMix" } });

    for (auto && delay : m_delays) {
        delay.fbLpf.setMode(CascadedSvf::Mode::LowPass);
        delay.fbHpf.setMode(CascadedSvf::Mode::HighPass);
        delay.fbLpf.setResonance(0.0);
        delay.fbHpf.setResonance(0.0);
    }

    m_wetLpfL.setMode(CascadedSvf::Mode::LowPass);
    m_wetLpfR.setMode(CascadedSvf::Mode::LowPass);
    m_wetHpfL.setMode(CascadedSvf::Mode::HighPass);
    m_wetHpfR.setMode(CascadedSvf::Mode::HighPass);

    ReverbEffect::syncParameters();
}

void ReverbEffect::process(double & left, double & right)
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

    double input = (dryL + dryR) * 0.5;

    if (!m_preDelayBuffer.empty()) {
        const double delayedInput = m_preDelayBuffer[m_preDelayWritePos];
        m_preDelayBuffer[m_preDelayWritePos] = input;
        m_preDelayWritePos = (m_preDelayWritePos + 1) % static_cast<uint32_t>(m_preDelayBuffer.size());
        input = delayedInput;
    }

    std::array<double, NumDelays> delayOutputs;
    for (size_t i = 0; i < NumDelays; i++) {
        if (m_delays[i].buffer.empty()) {
            delayOutputs[i] = 0.0;
            continue;
        }
        delayOutputs[i] = m_delays[i].buffer[m_delays[i].writePos];
    }

    std::array<double, NumDelays> feedbackSignals;
    for (size_t i = 0; i < NumDelays; i++) {
        if (m_delays[i].buffer.empty()) {
            feedbackSignals[i] = 0.0;
            continue;
        }

        const double dOut = delayOutputs[i];
        m_delays[i].lpState = dOut + static_cast<double>(m_damping) * (m_delays[i].lpState - dOut);

        if (std::abs(m_delays[i].lpState) < 1.0e-15) {
            m_delays[i].lpState = 0.0;
        }

        double filtered = m_delays[i].lpState;
        if (m_hpfCutoff > 0.001f) {
            filtered = m_delays[i].fbHpf.process(filtered);
        }
        if (m_lpfCutoff < 0.999f) {
            filtered = m_delays[i].fbLpf.process(filtered);
        }
        feedbackSignals[i] = filtered;
    }

    double sum = 0.0;
    for (size_t i = 0; i < NumDelays; i++) {
        sum += feedbackSignals[i];
    }
    const double average = sum * (2.0 / static_cast<double>(NumDelays));

    static constexpr std::array<double, NumDelays> inputGains = { 0.82, -0.74, 0.68, -0.61, 0.56, -0.51, 0.47, -0.43 };
    for (size_t i = 0; i < NumDelays; i++) {
        if (m_delays[i].buffer.empty()) {
            continue;
        }
        const double feedback = feedbackSignals[i] - average;
        m_delays[i].buffer[m_delays[i].writePos] = input * inputGains[i] + feedback * m_delays[i].feedback;
        m_delays[i].writePos = (m_delays[i].writePos + 1) % m_delays[i].size;
    }

    double wetL = (delayOutputs[0] - delayOutputs[2] + delayOutputs[4] - delayOutputs[6]) * 0.25;
    double wetR = (delayOutputs[1] - delayOutputs[3] + delayOutputs[5] - delayOutputs[7]) * 0.25;

    applyWetFilters(wetL, wetR);

    const double mid = (wetL + wetR) * 0.5;
    const double side = (wetL - wetR) * 0.5;
    wetL = mid + side * static_cast<double>(m_width);
    wetR = mid - side * static_cast<double>(m_width);

    left = dryL + wetL * static_cast<double>(m_mix);
    right = dryR + wetR * static_cast<double>(m_mix);
}

void ReverbEffect::process(AudioContext & context)
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

    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void ReverbEffect::reset()
{
    for (auto && dl : m_delays) {
        dl.reset();
    }
    std::fill(m_preDelayBuffer.begin(), m_preDelayBuffer.end(), 0.0);
    m_preDelayWritePos = 0;
    m_wetLpfL.reset();
    m_wetLpfR.reset();
    m_wetHpfL.reset();
    m_wetHpfR.reset();
}

void ReverbEffect::sync()
{
    m_shouldUpdateBuffers = true;
}

std::string ReverbEffect::typeIdString()
{
    return "47a2e2d0-1e5e-4f3a-9c6a-6a5b2d7e8f1a";
}

std::string ReverbEffect::type() const
{
    return Constants::RackEffectType::reverb().toStdString();
}

std::string ReverbEffect::typeId() const
{
    return typeIdString();
}

void ReverbEffect::setSize(float size)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySize().toStdString()); p) {
        p->get().setValue(size);
        m_shouldUpdateBuffers = true;
    }
}

float ReverbEffect::size() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySize().toStdString()); p) {
        return p->get().value();
    }
    return 0.5f;
}

void ReverbEffect::setDecay(float decay)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        p->get().setValue(decay);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::decay() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        return p->get().value();
    }
    return 0.15f;
}

void ReverbEffect::setDamping(float damping)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDamping().toStdString()); p) {
        p->get().setValue(damping);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::damping() const
{
    return m_damping;
}

void ReverbEffect::setPreDelay(float ms)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString()); p) {
        p->get().setValue(ms);
        m_shouldUpdateBuffers = true;
    }
}

float ReverbEffect::preDelay() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString()); p) {
        return p->get().value();
    }
    return 0.04f;
}

void ReverbEffect::setMix(float mix)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        p->get().setValue(mix);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::mix() const
{
    return m_mix;
}

void ReverbEffect::setWidth(float width)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyWidth().toStdString()); p) {
        p->get().setValue(width);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::width() const
{
    return m_width;
}

void ReverbEffect::setLpfCutoff(float cutoff)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        p->get().setValue(cutoff);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::lpfCutoff() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        return p->get().value();
    }
    return 0.85f;
}

void ReverbEffect::setHpfCutoff(float cutoff)
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        p->get().setValue(cutoff);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::hpfCutoff() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        return p->get().value();
    }
    return 0.2f;
}

void ReverbEffect::applyPreset(Preset preset)
{
    switch (preset) {
    case Preset::Hall:
        setSize(0.8f);
        setDecay(3000.0f / 10000.0f);
        setDamping(0.3f);
        setPreDelay(30.0f / 500.0f);
        setWidth(1.2f / 2.0f);
        setLpfCutoff(0.86f);
        setHpfCutoff(0.18f);
        break;
    case Preset::LargeRoom:
        setSize(0.6f);
        setDecay(1500.0f / 10000.0f);
        setDamping(0.4f);
        setPreDelay(15.0f / 500.0f);
        setWidth(1.0f / 2.0f);
        setLpfCutoff(0.82f);
        setHpfCutoff(0.2f);
        break;
    case Preset::SmallRoom:
        setSize(0.3f);
        setDecay(600.0f / 10000.0f);
        setDamping(0.5f);
        setPreDelay(5.0f / 500.0f);
        setWidth(0.8f / 2.0f);
        setLpfCutoff(0.78f);
        setHpfCutoff(0.24f);
        break;
    case Preset::Plate:
        setSize(0.5f);
        setDecay(2000.0f / 10000.0f);
        setDamping(0.2f);
        setPreDelay(10.0f / 500.0f);
        setWidth(1.5f / 2.0f);
        setLpfCutoff(0.9f);
        setHpfCutoff(0.2f);
        break;
    case Preset::Cathedral:
        setSize(1.0f);
        setDecay(8000.0f / 10000.0f);
        setDamping(0.2f);
        setPreDelay(50.0f / 500.0f);
        setWidth(2.0f / 2.0f);
        setLpfCutoff(0.84f);
        setHpfCutoff(0.16f);
        break;
    case Preset::Basement:
        setSize(0.2f);
        setDecay(400.0f / 10000.0f);
        setDamping(0.7f);
        setPreDelay(2.0f / 500.0f);
        setWidth(0.5f / 2.0f);
        setLpfCutoff(0.68f);
        setHpfCutoff(0.3f);
        break;
    case Preset::Tunnel:
        setSize(0.9f);
        setDecay(5000.0f / 10000.0f);
        setDamping(0.1f);
        setPreDelay(40.0f / 500.0f);
        setWidth(0.6f / 2.0f);
        setLpfCutoff(0.88f);
        setHpfCutoff(0.18f);
        break;
    case Preset::Spring:
        setSize(0.4f);
        setDecay(1200.0f / 10000.0f);
        setDamping(0.4f);
        setPreDelay(5.0f / 500.0f);
        setWidth(1.0f / 2.0f);
        setLpfCutoff(0.72f);
        setHpfCutoff(0.26f);
        break;
    }
}

std::string ReverbEffect::presetToString(Preset preset)
{
    switch (preset) {
    case Preset::Hall:
        return "Hall";
    case Preset::LargeRoom:
        return "Large Room";
    case Preset::SmallRoom:
        return "Small Room";
    case Preset::Plate:
        return "Plate";
    case Preset::Cathedral:
        return "Cathedral";
    case Preset::Basement:
        return "Basement";
    case Preset::Tunnel:
        return "Tunnel";
    case Preset::Spring:
        return "Spring";
    }
    return "Hall";
}

ReverbEffect::Preset ReverbEffect::stringToPreset(const std::string & presetName)
{
    if (presetName == "Hall")
        return Preset::Hall;
    if (presetName == "Large Room")
        return Preset::LargeRoom;
    if (presetName == "Small Room")
        return Preset::SmallRoom;
    if (presetName == "Plate")
        return Preset::Plate;
    if (presetName == "Cathedral")
        return Preset::Cathedral;
    if (presetName == "Basement")
        return Preset::Basement;
    if (presetName == "Tunnel")
        return Preset::Tunnel;
    if (presetName == "Spring")
        return Preset::Spring;
    return Preset::Hall;
}

std::vector<std::string> ReverbEffect::presetNames()
{
    return { "Hall", "Large Room", "Small Room", "Plate", "Cathedral", "Basement", "Tunnel", "Spring" };
}

void ReverbEffect::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySize().toStdString()); p) {
        m_size = std::clamp(p->get().value(), 0.01f, 1.0f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        m_decayMs = p->get().value() * 10000.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDamping().toStdString()); p) {
        m_damping = std::clamp(p->get().value(), 0.0f, 0.9f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString()); p) {
        m_preDelayMs = p->get().value() * 500.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyWidth().toStdString()); p) {
        m_width = p->get().value() * 2.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        m_mix = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = std::clamp(p->get().value(), 0.0f, 1.0f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = std::clamp(p->get().value(), 0.0f, 1.0f);
    }

    updateFilters();
}

void ReverbEffect::updateBuffers()
{
    if (m_sampleRate <= 0) {
        return;
    }
    m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);

    static const std::array<float, NumDelays> baseLengths = { 1277.0f, 1493.0f, 1723.0f, 1999.0f, 2293.0f, 2551.0f, 2879.0f, 3253.0f };
    const float rateScale = static_cast<float>(m_sampleRate) / 44100.0f;

    const float t60 = std::max(0.1f, m_decayMs / 1000.0f);

    for (size_t i = 0; i < NumDelays; i++) {
        uint32_t newSize = static_cast<uint32_t>(baseLengths[i] * rateScale * (0.5f + m_size));
        if (newSize < 100) {
            newSize = 100;
        }

        if (newSize != m_delays[i].size) {
            m_delays[i].buffer.assign(newSize, 0.0);
            m_delays[i].size = newSize;
            m_delays[i].writePos = 0;
            m_delays[i].lpState = 0.0;
            m_delays[i].fbLpf.reset();
            m_delays[i].fbHpf.reset();
        }

        m_delays[i].feedback = static_cast<double>(std::min(0.985f, std::pow(10.0f, -3.0f * static_cast<float>(newSize) / (t60 * static_cast<float>(m_sampleRate)))));
    }

    const uint32_t preDelaySize = std::max(1u, static_cast<uint32_t>(m_preDelayMs * static_cast<float>(m_sampleRate) / 1000.0f));
    if (preDelaySize != m_preDelayBuffer.size()) {
        m_preDelayBuffer.assign(preDelaySize, 0.0);
        m_preDelayWritePos = 0;
    }
}

void ReverbEffect::updateFilters()
{
    for (auto & delay : m_delays) {
        delay.fbLpf.setSampleRate(m_sampleRate);
        delay.fbHpf.setSampleRate(m_sampleRate);
        delay.fbLpf.setCutoff(static_cast<double>(m_lpfCutoff));
        delay.fbHpf.setCutoff(static_cast<double>(m_hpfCutoff));
    }

    m_wetLpfL.setSampleRate(m_sampleRate);
    m_wetLpfR.setSampleRate(m_sampleRate);
    m_wetHpfL.setSampleRate(m_sampleRate);
    m_wetHpfR.setSampleRate(m_sampleRate);
    m_wetLpfL.setCutoff(static_cast<double>(m_lpfCutoff));
    m_wetLpfR.setCutoff(static_cast<double>(m_lpfCutoff));
    m_wetHpfL.setCutoff(static_cast<double>(m_hpfCutoff));
    m_wetHpfR.setCutoff(static_cast<double>(m_hpfCutoff));
}

void ReverbEffect::applyWetFilters(double & wetL, double & wetR)
{
    if (m_hpfCutoff > 0.001f) {
        wetL = m_wetHpfL.process(wetL);
        wetR = m_wetHpfR.process(wetR);
    }
    if (m_lpfCutoff < 0.999f) {
        wetL = m_wetLpfL.process(wetL);
        wetR = m_wetLpfR.process(wetR);
    }
}

} // namespace noteahead
