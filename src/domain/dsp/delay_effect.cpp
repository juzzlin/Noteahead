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

#include "delay_effect.hpp"

#include "common/constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

DelayEffect::DelayEffect()
{
    updateBuffers();
}

void DelayEffect::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) > 0.1) {
        DspComponent::setSampleRate(sampleRate);
        updateBuffers();
    }
}

void DelayEffect::setType(Type type)
{
    m_type = type;
}

void DelayEffect::setTime(double seconds)
{
    m_time = std::clamp(seconds, 0.001, 2.0);
}

void DelayEffect::setFeedback(double feedback)
{
    m_feedback = std::clamp(feedback, 0.0, 0.95);
}

void DelayEffect::setDepth(double depth)
{
    m_depth = std::clamp(depth, 0.0, 1.0);
}

void DelayEffect::setMix(double mix)
{
    m_mix = std::clamp(mix, 0.0, 1.0);
}

void DelayEffect::setBpm(double bpm)
{
    m_bpm = std::max(1.0, bpm);
}

void DelayEffect::setSync(bool sync)
{
    m_sync = sync;
}

void DelayEffect::setSyncDivision(double division)
{
    m_syncDivision = division;
}

double DelayEffect::getDelaySamples() const
{
    if (m_sync) {
        // beat duration = 60 / BPM
        // division = 0.25 (1/4), 0.125 (1/8) etc.
        return (60.0 / m_bpm) * m_syncDivision * m_sampleRate * 4.0; // *4 because division 0.25 is 1/4 note
    }
    return m_time * m_sampleRate;
}

void DelayEffect::updateBuffers()
{
    // Max 3 seconds buffer
    size_t size = static_cast<size_t>(m_sampleRate * 3.0);
    m_bufferL.assign(size, 0.0f);
    m_bufferR.assign(size, 0.0f);
    m_writePos = 0;
}

void DelayEffect::reset()
{
    std::fill(m_bufferL.begin(), m_bufferL.end(), 0.0f);
    std::fill(m_bufferR.begin(), m_bufferR.end(), 0.0f);
    m_lpStateL = m_lpStateR = 0.0f;
    m_hpStateL = m_hpStateR = 0.0f;
}

void DelayEffect::process(float & left, float & right)
{
    if (m_mix <= Constants::minEffectLevel()) {
        return;
    }

    const double delaySamples = getDelaySamples();

    // 1. Read from buffers
    float outL = 0.0f;
    float outR = 0.0f;

    if (m_type == Type::PingPong) {
        outL = readFromBuffer(m_bufferR, delaySamples);
        outR = readFromBuffer(m_bufferL, delaySamples);
    } else {
        outL = readFromBuffer(m_bufferL, delaySamples);
        outR = readFromBuffer(m_bufferR, delaySamples);
    }

    // 2. Feedback and Character Processing
    float fbL = outL * static_cast<float>(m_feedback);
    float fbR = outR * static_cast<float>(m_feedback);

    applyTapeSaturation(fbL, fbR);

    // 3. Routing and Write-back
    updateWriteBuffer(left, right, fbL, fbR, outL, outR);

    // 4. Mix
    applyMix(left, right, outL, outR);
}

float DelayEffect::readFromBuffer(const std::vector<float> & buffer, double delay) const
{
    const size_t bufSize = buffer.size();
    double readPos = static_cast<double>(m_writePos) - delay;
    while (readPos < 0) {
        readPos += bufSize;
    }

    const size_t i0 = static_cast<size_t>(readPos) % bufSize;
    const size_t i1 = (i0 + 1) % bufSize;
    const float frac = static_cast<float>(readPos - static_cast<double>(i0));

    return buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
}

void DelayEffect::applyTapeSaturation(float & fbL, float & fbR)
{
    if (m_type == Type::Tape) {
        m_lpStateL += 0.3f * (fbL - m_lpStateL);
        m_lpStateR += 0.3f * (fbR - m_lpStateR);
        const float saturation = 1.0f + (static_cast<float>(m_depth) * 2.0f);
        fbL = std::tanh(m_lpStateL * saturation);
        fbR = std::tanh(m_lpStateR * saturation);
    }
}

void DelayEffect::updateWriteBuffer(float inputL, float inputR, float fbL, float fbR, float & outL, float & outR)
{
    (void)outL;
    (void)outR;

    const size_t bufSize = m_bufferL.size();

    if (m_type == Type::Mono) {
        const float monoInput = (inputL + inputR) * 0.5f;
        inputL = inputR = monoInput;
    }

    if (m_type == Type::PingPong) {
        // Depth controls width
        const float inL = inputL + inputR * (1.0f - static_cast<float>(m_depth));
        const float inR = inputR * (1.0f - static_cast<float>(m_depth));
        m_bufferL[m_writePos] = (inL + fbR) * static_cast<float>(m_feedback);
        m_bufferR[m_writePos] = (inR + fbL) * static_cast<float>(m_feedback);
    } else {
        m_bufferL[m_writePos] = (inputL + fbL) * static_cast<float>(m_feedback);
        m_bufferR[m_writePos] = (inputR + fbR) * static_cast<float>(m_feedback);
    }

    m_writePos = (m_writePos + 1) % bufSize;
}

void DelayEffect::applyMix(float & left, float & right, float outL, float outR) const
{
    // Mix: Unity-dry crossfade strategy
    const float dry = std::clamp(2.0f * (1.0f - static_cast<float>(m_mix)), 0.0f, 1.0f);
    const float wet = std::clamp(2.0f * static_cast<float>(m_mix), 0.0f, 1.0f);
    left = left * dry + outL * wet;
    right = right * dry + outR * wet;
}

} // namespace noteahead
