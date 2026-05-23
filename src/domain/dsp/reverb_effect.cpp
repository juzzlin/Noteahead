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

namespace noteahead {

ReverbEffect::ReverbEffect()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyReverbSize().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReverbDecay().toStdString(), 0.15f, 0, 10000, 1500 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReverbDamping().toStdString(), 0.3f, 0, 100, 30 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReverbPreDelay().toStdString(), 0.04f, 0, 500, 20 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReverbWidth().toStdString(), 0.5f, 0, 200, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyReverbMix().toStdString(), 0.0f, 0, 100, 0 });
    ReverbEffect::syncParameters();
}

void ReverbEffect::process(float & left, float & right)
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

    const float dryL = left;
    const float dryR = right;

    // Mono input for reverb
    float input = (dryL + dryR) * 0.5f;

    // Pre-delay
    if (!m_preDelayBuffer.empty()) {
        const float delayedInput = m_preDelayBuffer[m_preDelayWritePos];
        m_preDelayBuffer[m_preDelayWritePos] = input;
        m_preDelayWritePos = (m_preDelayWritePos + 1) % static_cast<uint32_t>(m_preDelayBuffer.size());
        input = delayedInput;
    }

    // FDN Processing
    std::array<float, NumDelays> delayOutputs;
    for (int i = 0; i < NumDelays; i++) {
        if (m_delays[i].buffer.empty()) {
            delayOutputs[i] = 0.0f;
            continue;
        }
        delayOutputs[i] = m_delays[i].buffer[m_delays[i].writePos];
    }

    // Apply damping to delay line outputs
    for (int i = 0; i < NumDelays; i++) {
        if (m_delays[i].buffer.empty()) {
            continue;
        }
        const float dOut = delayOutputs[i];
        m_delays[i].lpState = dOut + m_damping * (m_delays[i].lpState - dOut);

        // Denormal protection
        if (std::abs(m_delays[i].lpState) < 1.0e-15f) {
            m_delays[i].lpState = 0.0f;
        }
    }

    // Householder reflection for N=4
    float sum = 0.0f;
    for (int i = 0; i < NumDelays; i++) {
        sum += m_delays[i].lpState;
    }
    const float average = sum * (2.0f / static_cast<float>(NumDelays));

    for (int i = 0; i < NumDelays; i++) {
        if (m_delays[i].buffer.empty()) {
            continue;
        }
        const float feedback = m_delays[i].lpState - average;
        // Inject input + mixed feedback into delay lines
        m_delays[i].buffer[m_delays[i].writePos] = input + feedback * m_delays[i].feedback;
        m_delays[i].writePos = (m_delays[i].writePos + 1) % m_delays[i].size;
    }

    // Stereo Output from delay lines (using different combinations for decorrelation)
    float wetL = (delayOutputs[0] + delayOutputs[2]) * 0.5f;
    float wetR = (delayOutputs[1] + delayOutputs[3]) * 0.5f;

    // Apply width
    const float mid = (wetL + wetR) * 0.5f;
    const float side = (wetL - wetR) * 0.5f;
    wetL = mid + side * m_width;
    wetR = mid - side * m_width;

    // Mix
    left = dryL + wetL * m_mix;
    right = dryR + wetR * m_mix;
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
    for (auto & dl : m_delays) {
        dl.reset();
    }
    std::fill(m_preDelayBuffer.begin(), m_preDelayBuffer.end(), 0.0f);
    m_preDelayWritePos = 0;
}

void ReverbEffect::sync()
{
    m_shouldUpdateBuffers = true;
}

void ReverbEffect::setSize(float size)
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbSize().toStdString()); p) {
        p->get().setValue(size);
        m_shouldUpdateBuffers = true;
    }
}

float ReverbEffect::size() const
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbSize().toStdString()); p) {
        return p->get().value();
    }
    return 0.5f;
}

void ReverbEffect::setDecay(float decay)
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbDecay().toStdString()); p) {
        p->get().setValue(decay);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::decay() const
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbDecay().toStdString()); p) {
        return p->get().value();
    }
    return 0.15f;
}

void ReverbEffect::setDamping(float damping)
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbDamping().toStdString()); p) {
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
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbPreDelay().toStdString()); p) {
        p->get().setValue(ms);
        m_shouldUpdateBuffers = true;
    }
}

float ReverbEffect::preDelay() const
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbPreDelay().toStdString()); p) {
        return p->get().value();
    }
    return 0.04f;
}

void ReverbEffect::setMix(float mix)
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbMix().toStdString()); p) {
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
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbWidth().toStdString()); p) {
        p->get().setValue(width);
        m_shouldSyncParameters = true;
    }
}

float ReverbEffect::width() const
{
    return m_width;
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
        break;
    case Preset::LargeRoom:
        setSize(0.6f);
        setDecay(1500.0f / 10000.0f);
        setDamping(0.4f);
        setPreDelay(15.0f / 500.0f);
        setWidth(1.0f / 2.0f);
        break;
    case Preset::SmallRoom:
        setSize(0.3f);
        setDecay(600.0f / 10000.0f);
        setDamping(0.5f);
        setPreDelay(5.0f / 500.0f);
        setWidth(0.8f / 2.0f);
        break;
    case Preset::Plate:
        setSize(0.5f);
        setDecay(2000.0f / 10000.0f);
        setDamping(0.2f);
        setPreDelay(10.0f / 500.0f);
        setWidth(1.5f / 2.0f);
        break;
    case Preset::Cathedral:
        setSize(1.0f);
        setDecay(8000.0f / 10000.0f);
        setDamping(0.2f);
        setPreDelay(50.0f / 500.0f);
        setWidth(2.0f / 2.0f);
        break;
    case Preset::Basement:
        setSize(0.2f);
        setDecay(400.0f / 10000.0f);
        setDamping(0.7f);
        setPreDelay(2.0f / 500.0f);
        setWidth(0.5f / 2.0f);
        break;
    case Preset::Tunnel:
        setSize(0.9f);
        setDecay(5000.0f / 10000.0f);
        setDamping(0.1f);
        setPreDelay(40.0f / 500.0f);
        setWidth(0.6f / 2.0f);
        break;
    case Preset::Spring:
        setSize(0.4f);
        setDecay(1200.0f / 10000.0f);
        setDamping(0.4f);
        setPreDelay(5.0f / 500.0f);
        setWidth(1.0f / 2.0f);
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
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbSize().toStdString()); p) {
        m_size = std::clamp(p->get().value(), 0.01f, 1.0f);
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbDecay().toStdString()); p) {
        m_decayMs = p->get().value() * 10000.0f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbDamping().toStdString()); p) {
        m_damping = std::clamp(p->get().value(), 0.0f, 0.9f);
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbPreDelay().toStdString()); p) {
        m_preDelayMs = p->get().value() * 500.0f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbWidth().toStdString()); p) {
        m_width = p->get().value() * 2.0f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyReverbMix().toStdString()); p) {
        m_mix = p->get().value();
    }
}

void ReverbEffect::updateBuffers()
{
    if (m_sampleRate <= 0) {
        return;
    }
    m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);

    // Prime-like delay lengths for better diffusion and less resonance (metallic sound)
    static const std::array<float, NumDelays> baseLengths = { 1103.0f, 1373.0f, 1741.0f, 2131.0f };
    const float rateScale = static_cast<float>(m_sampleRate) / 44100.0f;

    const float t60 = std::max(0.1f, m_decayMs / 1000.0f);

    for (int i = 0; i < NumDelays; i++) {
        // Size scale: 0.5 to 1.5 of base lengths
        uint32_t newSize = static_cast<uint32_t>(baseLengths[i] * rateScale * (0.5f + m_size));
        if (newSize < 100) {
            newSize = 100;
        }

        if (newSize != m_delays[i].size) {
            m_delays[i].buffer.assign(newSize, 0.0f);
            m_delays[i].size = newSize;
            m_delays[i].writePos = 0;
            m_delays[i].lpState = 0.0f;
        }

        // Feedback calculation for desired T60 decay time
        m_delays[i].feedback = std::pow(10.0f, -3.0f * static_cast<float>(newSize) / (t60 * static_cast<float>(m_sampleRate)));
    }

    const uint32_t preDelaySize = std::max(1u, static_cast<uint32_t>(m_preDelayMs * static_cast<float>(m_sampleRate) / 1000.0f));
    if (preDelaySize != m_preDelayBuffer.size()) {
        m_preDelayBuffer.assign(preDelaySize, 0.0f);
        m_preDelayWritePos = 0;
    }
}

} // namespace noteahead
