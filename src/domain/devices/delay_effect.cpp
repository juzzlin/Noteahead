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

#include "../dsp/audio_context.hpp"
#include "../../common/constants.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

DelayEffect::DelayEffect()
{
    m_fbLpfL.setMode(CascadedSvf::Mode::LowPass);
    m_fbLpfR.setMode(CascadedSvf::Mode::LowPass);
    m_fbHpfL.setMode(CascadedSvf::Mode::HighPass);
    m_fbHpfR.setMode(CascadedSvf::Mode::HighPass);
}

void DelayEffect::process(float & left, float & right)
{
    if (m_bufferL.empty()) {
        return;
    }

    // Ensure filters are updated even if process(AudioContext&) wasn't called (e.g. per-sample usage)
    m_fbLpfL.setCutoff(m_feedbackLpfCutoff);
    m_fbLpfR.setCutoff(m_feedbackLpfCutoff);
    m_fbHpfL.setCutoff(std::max(0.001f, m_feedbackHpfCutoff));
    m_fbHpfR.setCutoff(std::max(0.001f, m_feedbackHpfCutoff));

    const double delaySamples = calculateDelaySamples();

    // 1. Read from buffers
    float outL = readFromBuffer(m_bufferL, delaySamples);
    float outR = readFromBuffer(m_bufferR, delaySamples);

    // 2. Feedback and Character Processing
    float fbL = outL;
    float fbR = outR;

    applyTapeSaturation(fbL, fbR);
    applyFeedbackFilters(fbL, fbR);

    // 3. Routing and Write-back
    updateWriteBuffer(left, right, fbL, fbR, outL, outR);

    // 4. Mix
    applyMix(left, right, outL, outR);
}

double DelayEffect::calculateDelaySamples() const
{
    const size_t bufSize = m_bufferL.size();
    if (bufSize < 2) {
        return 0.0;
    }
    const float delayTime = m_sync ? (60.0f / m_bpm) * m_syncDivision * 4.0f : m_time;
    const uint32_t sampleRate = static_cast<uint32_t>(m_sampleRate);
    return std::clamp(static_cast<double>(delayTime) * sampleRate, 1.0, static_cast<double>(bufSize - 2));
}

float DelayEffect::readFromBuffer(const std::vector<float> & buffer, double delay) const
{
    const size_t bufSize = buffer.size();
    const double bufSizeD = static_cast<double>(bufSize);
    double readPos = static_cast<double>(m_writePos) - delay;
    while (readPos < 0.0) {
        readPos += bufSizeD;
    }
    while (readPos >= bufSizeD) {
        readPos -= bufSizeD;
    }

    const size_t i0 = static_cast<size_t>(readPos) % bufSize;
    const size_t i1 = (i0 + 1) % bufSize;
    const float frac = static_cast<float>(readPos - static_cast<double>(i0));

    return buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
}

void DelayEffect::applyFeedbackFilters(float & fbL, float & fbR)
{
    if (m_feedbackLpfCutoff < 0.999f) {
        fbL = m_fbLpfL.process(fbL);
        fbR = m_fbLpfR.process(fbR);
    }
    if (m_feedbackHpfCutoff > 0.001f) {
        fbL = m_fbHpfL.process(fbL);
        fbR = m_fbHpfR.process(fbR);
    }
}

void DelayEffect::applyTapeSaturation(float & fbL, float & fbR)
{
    if (m_type != Type::Tape) {
        return;
    }

    // Tape: Dark, saturated, slightly compressed. Depth controls saturation.
    m_lpStateL += 0.08f * (fbL - m_lpStateL);
    m_lpStateR += 0.08f * (fbR - m_lpStateR);
    const float saturation = 1.0f + (m_depth * 2.0f);
    fbL = std::tanh(m_lpStateL * saturation);
    fbR = std::tanh(m_lpStateR * saturation);

    // Denormal protection for feedback states
    if (std::abs(m_lpStateL) < 1.0e-15f) {
        m_lpStateL = 0.0f;
    }
    if (std::abs(m_lpStateR) < 1.0e-15f) {
        m_lpStateR = 0.0f;
    }
}

void DelayEffect::updateWriteBuffer(float inputL, float inputR, float fbL, float fbR, float & outL, float & outR)
{
    const size_t bufSize = m_bufferL.size();

    if (m_type == Type::PingPong) {
        // Ping-Pong: Depth controls stereo width/bounce amount.
        const float inL = inputL + inputR * (1.0f - m_depth);
        const float inR = inputR * (1.0f - m_depth);

        m_bufferL[m_writePos] = (inL + fbR) * m_feedback;
        m_bufferR[m_writePos] = (inR + fbL) * m_feedback;
    } else if (m_type == Type::Mono) {
        // Sum input and feedback to mono delay line
        const float monoInput = (inputL + inputR) * 0.5f;
        const float monoFb = (fbL + fbR) * 0.5f;
        m_bufferL[m_writePos] = m_bufferR[m_writePos] = (monoInput + monoFb) * m_feedback;
        outL = outR = (outL + outR) * 0.5f;
    } else {
        // Normal Stereo
        m_bufferL[m_writePos] = (inputL + fbL) * m_feedback;
        m_bufferR[m_writePos] = (inputR + fbR) * m_feedback;
    }

    m_writePos = (m_writePos + 1) % bufSize;
}

void DelayEffect::applyMix(float & left, float & right, float outL, float outR) const
{
    if (m_mix <= Constants::minEffectLevel()) {
        return;
    }

    // Mix: Use a unity-dry crossfade strategy (0% to 50% Dry is 100%, Wet fades in)
    const float dry = std::clamp(2.0f * (1.0f - m_mix), 0.0f, 1.0f);
    const float wet = std::clamp(2.0f * m_mix, 0.0f, 1.0f);
    left = left * dry + outL * wet;
    right = right * dry + outR * wet;
}

void DelayEffect::process(AudioContext & context)
{
    const auto sampleRate = static_cast<uint32_t>(m_sampleRate);
    if ((sampleRate != m_lastSampleRate || m_bufferL.empty()) && sampleRate > 0) {
        updateBuffers(sampleRate);
        m_lastSampleRate = sampleRate;
        m_fbLpfL.setSampleRate(m_sampleRate);
        m_fbLpfR.setSampleRate(m_sampleRate);
        m_fbHpfL.setSampleRate(m_sampleRate);
        m_fbHpfR.setSampleRate(m_sampleRate);
    }

    m_fbLpfL.setCutoff(m_feedbackLpfCutoff);
    m_fbLpfR.setCutoff(m_feedbackLpfCutoff);
    m_fbHpfL.setCutoff(std::max(0.001f, m_feedbackHpfCutoff));
    m_fbHpfR.setCutoff(std::max(0.001f, m_feedbackHpfCutoff));

    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void DelayEffect::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) > 0.1 || m_bufferL.empty()) {
        DspComponent::setSampleRate(sampleRate);
        const auto rate = static_cast<uint32_t>(m_sampleRate);
        updateBuffers(rate);
        m_lastSampleRate = rate;
        m_fbLpfL.setSampleRate(m_sampleRate);
        m_fbLpfR.setSampleRate(m_sampleRate);
        m_fbHpfL.setSampleRate(m_sampleRate);
        m_fbHpfR.setSampleRate(m_sampleRate);
        
        m_fbLpfL.setCutoff(m_feedbackLpfCutoff);
        m_fbLpfR.setCutoff(m_feedbackLpfCutoff);
        m_fbHpfL.setCutoff(std::max(0.001f, m_feedbackHpfCutoff));
        m_fbHpfR.setCutoff(std::max(0.001f, m_feedbackHpfCutoff));
    }
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
void DelayEffect::setFeedback(float feedback) { m_feedback = std::clamp(feedback, 0.0f, 1.0f); }
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
