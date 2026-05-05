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
#include <cmath>
#include <algorithm>

namespace noteahead {

DelayEffect::DelayEffect()
{
    m_fbLpfL.setMode(CascadedSVF::Mode::LowPass);
    m_fbLpfR.setMode(CascadedSVF::Mode::LowPass);
    m_fbHpfL.setMode(CascadedSVF::Mode::HighPass);
    m_fbHpfR.setMode(CascadedSVF::Mode::HighPass);
}

void DelayEffect::process(float & left, float & right)
{
    const auto sampleRate = static_cast<uint32_t>(m_sampleRate);
    if (sampleRate != m_lastSampleRate) {
        updateBuffers(sampleRate);
        m_lastSampleRate = sampleRate;
        m_fbLpfL.setSampleRate(m_sampleRate);
        m_fbLpfR.setSampleRate(m_sampleRate);
        m_fbHpfL.setSampleRate(m_sampleRate);
        m_fbHpfR.setSampleRate(m_sampleRate);
    }

    if (m_mix <= 0.001f) return;

    float delayTime;
    if (m_sync) {
        delayTime = (60.0f / m_bpm) * m_syncDivision * 4.0f;
    } else {
        delayTime = m_time;
    }

    double delaySamples = static_cast<double>(delayTime) * sampleRate;
    size_t bufSize = m_bufferL.size();

    // Clamp delay samples to avoid reading out of bounds if buffer is too small
    delaySamples = std::clamp(delaySamples, 1.0, static_cast<double>(bufSize - 2));

    auto readFromBuffer = [&](const std::vector<float>& buf, double delay) {
        double readPos = static_cast<double>(m_writePos) - delay;
        while (readPos < 0) readPos += bufSize;
        
        size_t i0 = static_cast<size_t>(readPos) % bufSize;
        size_t i1 = (i0 + 1) % bufSize;
        float frac = static_cast<float>(readPos - static_cast<double>(static_cast<size_t>(readPos)));
        
        return buf[i0] * (1.0f - frac) + buf[i1] * frac;
    };

    // 1. Read from buffers
    float outL = readFromBuffer(m_bufferL, delaySamples);
    float outR = readFromBuffer(m_bufferR, delaySamples);

    // 2. Feedback and Character Processing
    float fbL = outL;
    float fbR = outR;

    if (m_type == Type::LowPass) {
        // Aggressive LowPass
        m_lpStateL += 0.15f * (fbL - m_lpStateL);
        m_lpStateR += 0.15f * (fbR - m_lpStateR);
        fbL = m_lpStateL;
        fbR = m_lpStateR;
    } else if (m_type == Type::HiPass) {
        // Aggressive HiPass
        m_hpStateL += 0.2f * (fbL - m_hpStateL);
        m_hpStateR += 0.2f * (fbR - m_hpStateR);
        fbL -= m_hpStateL;
        fbR -= m_hpStateR;
    } else if (m_type == Type::Tape) {
        // Tape: Dark, saturated, slightly compressed
        m_lpStateL += 0.08f * (fbL - m_lpStateL);
        m_lpStateR += 0.08f * (fbR - m_lpStateR);
        fbL = std::tanh(m_lpStateL * 1.5f);
        fbR = std::tanh(m_lpStateR * 1.5f);
    }

    // Apply feedback filters
    m_fbLpfL.setCutoff(m_feedbackLpfCutoff);
    m_fbLpfR.setCutoff(m_feedbackLpfCutoff);
    m_fbHpfL.setCutoff(m_feedbackHpfCutoff);
    m_fbHpfR.setCutoff(m_feedbackHpfCutoff);

    fbL = m_fbLpfL.process(fbL);
    fbR = m_fbLpfR.process(fbR);
    fbL = m_fbHpfL.process(fbL);
    fbR = m_fbHpfR.process(fbR);

    // 3. Routing and Write-back
    float inputL = left;
    float inputR = right;

    if (m_type == Type::PingPong) {
        // Ping-Pong: Depth controls how much of the input goes to one side vs both.
        // If Depth is 1.0, input only goes to Left channel of delay, then bounces.
        float inL = inputL + inputR * (1.0f - m_depth);
        float inR = inputR * (1.0f - m_depth);
        
        m_bufferL[m_writePos] = inL + fbR * m_feedback;
        m_bufferR[m_writePos] = inR + fbL * m_feedback;
    } else if (m_type == Type::Mono) {
        // Sum input and feedback to mono delay line
        float monoInput = (inputL + inputR) * 0.5f;
        float monoFb = (fbL + fbR) * 0.5f * m_feedback;
        m_bufferL[m_writePos] = m_bufferR[m_writePos] = monoInput + monoFb;
        outL = outR = (outL + outR) * 0.5f;
    } else {
        // Normal Stereo
        m_bufferL[m_writePos] = inputL + fbL * m_feedback;
        m_bufferR[m_writePos] = inputR + fbR * m_feedback;
    }

    m_writePos = (m_writePos + 1) % bufSize;

    // 4. Mix
    left = left * (1.0f - m_mix) + outL * m_mix;
    right = right * (1.0f - m_mix) + outR * m_mix;
}

void DelayEffect::reset()
{
    std::fill(m_bufferL.begin(), m_bufferL.end(), 0.0f);
    std::fill(m_bufferR.begin(), m_bufferR.end(), 0.0f);
    m_lpStateL = m_lpStateR = 0.0f;
    m_hpStateL = m_hpStateR = 0.0f;
    m_fbLpfL.reset();
    m_fbLpfR.reset();
    m_fbHpfL.reset();
    m_fbHpfR.reset();
}

void DelayEffect::setType(Type type) { m_type = type; }
void DelayEffect::setTime(float seconds) { m_time = std::clamp(seconds, 0.001f, 10.0f); }
void DelayEffect::setFeedback(float feedback) { m_feedback = std::clamp(feedback, 0.0f, 0.99f); }
void DelayEffect::setDepth(float depth) { m_depth = std::clamp(depth, 0.0f, 1.0f); }
void DelayEffect::setMix(float mix) { m_mix = std::clamp(mix, 0.0f, 1.0f); }
void DelayEffect::setBpm(float bpm) { m_bpm = std::max(1.0f, bpm); }
float DelayEffect::bpm() const { return m_bpm; }
void DelayEffect::setSync(bool sync) { m_sync = sync; }
void DelayEffect::setSyncDivision(float division) { m_syncDivision = division; }

void DelayEffect::setFeedbackLpf(float cutoff) { m_feedbackLpfCutoff = std::clamp(cutoff, 0.0f, 1.0f); }
float DelayEffect::feedbackLpf() const { return m_feedbackLpfCutoff; }
void DelayEffect::setFeedbackHpf(float cutoff) { m_feedbackHpfCutoff = std::clamp(cutoff, 0.0f, 1.0f); }
float DelayEffect::feedbackHpf() const { return m_feedbackHpfCutoff; }

void DelayEffect::updateBuffers(uint32_t sampleRate)
{
    size_t size = static_cast<size_t>(sampleRate * 10); // 10 seconds max
    m_bufferL.assign(size, 0.0f);
    m_bufferR.assign(size, 0.0f);
    m_writePos = 0;
}

} // namespace noteahead
