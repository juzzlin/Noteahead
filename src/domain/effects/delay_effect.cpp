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

#include "domain/effects/delay_effect.hpp"

#include "common/constants.hpp"
#include "domain/dsp/audio_context.hpp"

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

void DelayEffect::process(double & left, double & right)
{
    if (m_bufferL.empty()) {
        return;
    }

    // Ensure filters are updated even if process(AudioContext&) wasn't called (e.g. per-sample usage)
    updateFilters();

    const double delaySamples = calculateDelaySamples();

    // 1. Read from buffers (this is our WET signal)
    double outL = readFromBuffer(m_bufferL, delaySamples);
    double outR = readFromBuffer(m_bufferR, delaySamples);

    // 2. Feedback and Character Processing
    double fbL = outL;
    double fbR = outR;

    // 3. Routing and Write-back
    updateWriteBuffer(left, right, fbL, fbR, outL, outR);

    // 4. Mix
    applyMix(left, right, outL, outR);
}

void DelayEffect::updateFilters()
{
    m_fbLpfL.setCutoff(m_feedbackLpfCutoff);
    m_fbLpfR.setCutoff(m_feedbackLpfCutoff);
    m_fbHpfL.setCutoff(std::max(0.001, m_feedbackHpfCutoff));
    m_fbHpfR.setCutoff(std::max(0.001, m_feedbackHpfCutoff));
}

void DelayEffect::updateWriteBuffer(double inputL, double inputR, double fbL, double fbR, double & outL, double & outR)
{
    const size_t bufSize = m_bufferL.size();
    double writeL = inputL;
    double writeR = inputR;

    if (m_type == Type::PingPong) {
        // Ping-Pong: Depth controls stereo width/bounce amount.
        const double inL = inputL + inputR * (1.0 - m_depth);
        const double inR = inputR * (1.0 - m_depth);
        writeL = inL + fbR;
        writeR = inR + fbL;
    } else if (m_type == Type::Mono) {
        // Sum input and feedback to mono delay line
        const double monoInput = (inputL + inputR) * 0.5;
        const double monoFb = (fbL + fbR) * 0.5;
        writeL = writeR = monoInput + monoFb;
        outL = outR = (outL + outR) * 0.5;
    } else {
        // Normal Stereo
        writeL = inputL + fbL;
        writeR = inputR + fbR;
    }

    applyTapeSaturation(writeL, writeR);
    applyFeedbackFilters(writeL, writeR);

    m_bufferL[m_writePos] = writeL * m_feedback;
    m_bufferR[m_writePos] = writeR * m_feedback;

    m_writePos = (m_writePos + 1) % bufSize;
}

double DelayEffect::calculateDelaySamples() const
{
    const size_t bufSize = m_bufferL.size();
    if (bufSize < 2) {
        return 0.0;
    }
    const double delayTime = m_sync ? (60.0 / static_cast<double>(bpm())) * m_syncDivision * 4.0 : m_time;
    const uint32_t sampleRate = static_cast<uint32_t>(m_sampleRate);
    return std::clamp(delayTime * sampleRate, 1.0, static_cast<double>(bufSize - 2));
}

double DelayEffect::readFromBuffer(const std::vector<double> & buffer, double delay) const
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
    const double frac = readPos - static_cast<double>(i0);

    return buffer[i0] * (1.0 - frac) + buffer[i1] * frac;
}

void DelayEffect::applyFeedbackFilters(double & fbL, double & fbR)
{
    if (m_feedbackLpfCutoff < 0.999) {
        fbL = m_fbLpfL.process(fbL);
        fbR = m_fbLpfR.process(fbR);
    }
    if (m_feedbackHpfCutoff > 0.001) {
        fbL = m_fbHpfL.process(fbL);
        fbR = m_fbHpfR.process(fbR);
    }
}

std::string DelayEffect::typeIdString()
{
    return "7c2e3d0a-4f6b-4b2a-8c1d-1a2b3c4d5e6f";
}

std::string DelayEffect::type() const
{
    return Constants::RackEffectType::delay().toStdString();
}

std::string DelayEffect::typeId() const
{
    return typeIdString();
}

void DelayEffect::applyTapeSaturation(double & fbL, double & fbR)
{
    if (m_type != Type::Tape) {
        return;
    }

    // Tape: Dark, saturated, slightly compressed. Depth controls saturation.
    m_lpStateL += 0.2 * (fbL - m_lpStateL);
    m_lpStateR += 0.2 * (fbR - m_lpStateR);

    const double saturation = 1.0 + (m_depth * 2.0);
    fbL = std::tanh(m_lpStateL * saturation);
    fbR = std::tanh(m_lpStateR * saturation);

    // Denormal protection for feedback states
    if (std::abs(m_lpStateL) < 1.0e-15) {
        m_lpStateL = 0.0;
    }
    if (std::abs(m_lpStateR) < 1.0e-15) {
        m_lpStateR = 0.0;
    }
}

void DelayEffect::applyMix(double & left, double & right, double outL, double outR) const
{
    if (m_mix <= Constants::minEffectLevel()) {
        return;
    }

    // Mix: Use a unity-dry crossfade strategy (0% to 50% Dry is 100%, Wet fades in)
    const double dry = std::clamp(2.0 * (1.0 - m_mix), 0.0, 1.0);
    const double wet = std::clamp(2.0 * m_mix, 0.0, 1.0);
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

    updateFilters();

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

        updateFilters();
    }
}

void DelayEffect::reset()
{
    std::fill(m_bufferL.begin(), m_bufferL.end(), 0.0);
    std::fill(m_bufferR.begin(), m_bufferR.end(), 0.0);
    m_lpStateL = m_lpStateR = 0.0;
    m_hpStateL = m_hpStateR = 0.0;
    m_fbLpfL.reset();
    m_fbLpfR.reset();
    m_fbHpfL.reset();
    m_fbHpfR.reset();
}

void DelayEffect::setType(Type type)
{
    m_type = type;
}

void DelayEffect::setTime(double seconds)
{
    m_time = std::clamp(seconds, 0.001, 10.0);
}

void DelayEffect::setFeedback(double feedback)
{
    m_feedback = std::clamp(feedback, 0.0, 1.0);
}

void DelayEffect::setDepth(double depth)
{
    m_depth = std::clamp(depth, 0.0, 1.0);
}

void DelayEffect::setMix(double mix)
{
    m_mix = std::clamp(mix, 0.0, 1.0);
}

void DelayEffect::setSync(bool sync)
{
    m_sync = sync;
}

void DelayEffect::setSyncDivision(double division)
{
    m_syncDivision = division;
}

void DelayEffect::setFeedbackLpf(double cutoff)
{
    m_feedbackLpfCutoff = std::clamp(cutoff, 0.0, 1.0);
}

double DelayEffect::feedbackLpf() const
{
    return m_feedbackLpfCutoff;
}

void DelayEffect::setFeedbackHpf(double cutoff)
{
    m_feedbackHpfCutoff = std::clamp(cutoff, 0.0, 1.0);
}

double DelayEffect::feedbackHpf() const
{
    return m_feedbackHpfCutoff;
}

void DelayEffect::updateBuffers(uint32_t sampleRate)
{
    size_t size = static_cast<size_t>(sampleRate * 10); // 10 seconds max
    m_bufferL.assign(size, 0.0);
    m_bufferR.assign(size, 0.0);
    m_writePos = 0;
}

} // namespace noteahead
