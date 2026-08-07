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

#include "endless_reverb.hpp"

#include "../../common/constants.hpp"
#include "../dsp/audio_context.hpp"

#include <cmath>
#include <cstddef>
#include <numbers>

namespace noteahead {

namespace {
constexpr double twoPi = 2.0 * std::numbers::pi;
constexpr std::array<double, 8> baseLengths = { 1277.0, 1493.0, 1723.0, 1999.0, 2293.0, 2551.0, 2879.0, 3253.0 };
constexpr std::array<double, 8> inputGains = { 0.82, -0.74, 0.68, -0.61, 0.56, -0.51, 0.47, -0.43 };
constexpr std::array<double, 4> diffuserLengths = { 210.0, 158.0, 561.0, 410.0 };
} // namespace

EndlessReverb::EndlessReverb()
{
    addParameter({ Constants::NahdXml::xmlKeySize().toStdString(), 0.7f, 0, 10000, 7000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyDecay().toStdString(), 0.85f, 0, 10000, 8500, 100 });
    addParameter({ Constants::NahdXml::xmlKeyDamping().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyPreDelay().toStdString(), 0.04f, 0, 500, 20, 1 });
    addParameter({ Constants::NahdXml::xmlKeyDepth().toStdString(), 0.4f, 0, 10000, 4000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyRate().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyWidth().toStdString(), 0.5f, 0, 200, 100, 1 });
    addParameter({ Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 0.8f, 0, 10000, 8000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.2f, 0, 10000, 2000, 100 });
    addParameter({ Constants::NahdXml::xmlKeyMix().toStdString(), 0.0f, 0, 10000, 0, 100 });
    addParameter({ Constants::NahdXml::xmlKeyFreeze().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

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

    for (size_t i = 0; i < NumDelays; i++) {
        m_delays[i].lfoPhase = static_cast<double>(i) * twoPi / static_cast<double>(NumDelays);
    }

    EndlessReverb::syncParameters();
}

void EndlessReverb::processSample(double & left, double & right)
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

    renderSample(left, right);
}

void EndlessReverb::processBlock(AudioContext & context)
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
        renderSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void EndlessReverb::renderSample(double & left, double & right)
{
    if (m_delays[0].buffer.empty()) {
        return;
    }

    const double dryL = left;
    const double dryR = right;

    double input = (dryL + dryR) * 0.5;

    // Pre-delay
    if (!m_preDelayBuffer.empty()) {
        const double delayed = m_preDelayBuffer[m_preDelayWritePos];
        m_preDelayBuffer[m_preDelayWritePos] = input;
        m_preDelayWritePos = (m_preDelayWritePos + 1) % static_cast<uint32_t>(m_preDelayBuffer.size());
        input = delayed;
    }

    // Input diffusion
    for (auto && diffuser : m_diffusers) {
        input = diffuser.process(input);
    }

    // Freeze pins the network to a lossless, undamped, input-free state for an infinite tail.
    const double effFeedback = m_freeze ? 1.0 : static_cast<double>(m_feedback);
    const double effDamping = m_freeze ? 0.0 : static_cast<double>(m_damping);
    const double inputGainScale = m_freeze ? 0.0 : 1.0;

    // Modulated reads from each delay line.
    std::array<double, NumDelays> outs;
    for (size_t i = 0; i < NumDelays; i++) {
        auto && dl = m_delays[i];
        const double displacement = static_cast<double>(m_modDepth) * dl.modDepthSamples * std::sin(dl.lfoPhase);
        const double delaySamples = std::clamp(dl.nominalDelay + displacement, 1.0, static_cast<double>(dl.bufferLen) - 2.0);
        outs[i] = dl.read(delaySamples);
        dl.lfoPhase += dl.lfoInc;
        if (dl.lfoPhase >= twoPi) {
            dl.lfoPhase -= twoPi;
        }
    }

    // Damping and feedback-path filtering (bypassed while frozen to keep the tail from decaying).
    std::array<double, NumDelays> feedbackSignals;
    for (size_t i = 0; i < NumDelays; i++) {
        auto && dl = m_delays[i];
        dl.lpState = outs[i] + effDamping * (dl.lpState - outs[i]);
        if (std::abs(dl.lpState) < 1.0e-15) {
            dl.lpState = 0.0;
        }
        double filtered = dl.lpState;
        if (!m_freeze) {
            if (m_hpfCutoff > 0.001f) {
                filtered = dl.fbHpf.process(filtered);
            }
            if (m_lpfCutoff < 0.999f) {
                filtered = dl.fbLpf.process(filtered);
            }
        }
        feedbackSignals[i] = filtered;
    }

    // Householder feedback matrix (energy-preserving reflection) scaled by the global feedback.
    double sum = 0.0;
    for (size_t i = 0; i < NumDelays; i++) {
        sum += feedbackSignals[i];
    }
    const double average = sum * (2.0 / static_cast<double>(NumDelays));

    for (size_t i = 0; i < NumDelays; i++) {
        const double mixed = feedbackSignals[i] - average;
        m_delays[i].write(input * inputGains[i] * inputGainScale + mixed * effFeedback);
    }

    double wetL = (outs[0] - outs[2] + outs[4] - outs[6]) * 0.25;
    double wetR = (outs[1] - outs[3] + outs[5] - outs[7]) * 0.25;

    if (m_hpfCutoff > 0.001f) {
        wetL = m_wetHpfL.process(wetL);
        wetR = m_wetHpfR.process(wetR);
    }
    if (m_lpfCutoff < 0.999f) {
        wetL = m_wetLpfL.process(wetL);
        wetR = m_wetLpfR.process(wetR);
    }

    const double mid = (wetL + wetR) * 0.5;
    const double side = (wetL - wetR) * 0.5;
    wetL = mid + side * static_cast<double>(m_width);
    wetR = mid - side * static_cast<double>(m_width);

    left = dryL + wetL * static_cast<double>(m_mix);
    right = dryR + wetR * static_cast<double>(m_mix);
}

void EndlessReverb::reset()
{
    for (auto && dl : m_delays) {
        dl.reset();
    }
    for (size_t i = 0; i < NumDelays; i++) {
        m_delays[i].lfoPhase = static_cast<double>(i) * twoPi / static_cast<double>(NumDelays);
    }
    for (auto && diffuser : m_diffusers) {
        diffuser.reset();
    }
    std::fill(m_preDelayBuffer.begin(), m_preDelayBuffer.end(), 0.0);
    m_preDelayWritePos = 0;
    m_wetLpfL.reset();
    m_wetLpfR.reset();
    m_wetHpfL.reset();
    m_wetHpfR.reset();
}

void EndlessReverb::sync()
{
    m_shouldUpdateBuffers = true;
}

void EndlessReverb::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySize().toStdString()); p) {
        m_size = std::clamp(p->get().value(), 0.01f, 1.0f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        m_feedback = 0.7f + std::clamp(p->get().value(), 0.0f, 1.0f) * 0.295f; // 0.7 .. 0.995
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDamping().toStdString()); p) {
        m_damping = std::clamp(p->get().value(), 0.0f, 0.9f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString()); p) {
        m_preDelayMs = p->get().value() * 500.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDepth().toStdString()); p) {
        m_modDepth = std::clamp(p->get().value(), 0.0f, 1.0f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRate().toStdString()); p) {
        m_modRateHz = 0.05f + std::clamp(p->get().value(), 0.0f, 1.0f) * 3.0f; // 0.05 .. 3.05 Hz
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyWidth().toStdString()); p) {
        m_width = p->get().value() * 2.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()); p) {
        m_lpfCutoff = std::clamp(p->get().value(), 0.0f, 1.0f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) {
        m_hpfCutoff = std::clamp(p->get().value(), 0.0f, 1.0f);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        m_mix = p->get().value();
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFreeze().toStdString()); p) {
        m_freeze = p->get().value() > 0.5f;
    }

    updateFilters();
}

void EndlessReverb::updateBuffers()
{
    if (m_sampleRate <= 0) {
        return;
    }
    m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);

    const double rateScale = m_sampleRate / 44100.0;
    const double maxModSamples = 0.01 * m_sampleRate;
    const double sizeScale = 0.5 + static_cast<double>(m_size) * 3.0; // Cosmic scale (up to ~3.5x the base lengths).

    for (size_t i = 0; i < NumDelays; i++) {
        auto && dl = m_delays[i];
        const double nominal = baseLengths[i] * rateScale * sizeScale;
        const uint32_t newLen = static_cast<uint32_t>(nominal + maxModSamples + 4.0);
        if (newLen != dl.bufferLen) {
            dl.buffer.assign(newLen, 0.0);
            dl.bufferLen = newLen;
            dl.writePos = 0;
            dl.lpState = 0.0;
            dl.fbLpf.reset();
            dl.fbHpf.reset();
        }
        dl.nominalDelay = nominal;
        dl.modDepthSamples = maxModSamples;
        dl.lfoInc = twoPi * static_cast<double>(m_modRateHz) / m_sampleRate * (1.0 + 0.02 * static_cast<double>(i));
    }

    for (size_t i = 0; i < NumDiffusers; i++) {
        auto && ap = m_diffusers[i];
        const uint32_t newSize = std::max(1u, static_cast<uint32_t>(diffuserLengths[i] * rateScale));
        if (newSize != ap.size) {
            ap.buffer.assign(newSize, 0.0);
            ap.size = newSize;
            ap.writePos = 0;
        }
        ap.coeff = 0.7;
    }

    const uint32_t preDelaySize = std::max(1u, static_cast<uint32_t>(m_preDelayMs * static_cast<float>(m_sampleRate) / 1000.0f));
    if (preDelaySize != m_preDelayBuffer.size()) {
        m_preDelayBuffer.assign(preDelaySize, 0.0);
        m_preDelayWritePos = 0;
    }
}

void EndlessReverb::updateFilters()
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

std::string EndlessReverb::typeIdString()
{
    return "5d9e0b3c-2a7f-4c81-b6e4-3f1a8d0c9e2b";
}

std::string EndlessReverb::type() const
{
    return Constants::RackEffectType::endless().toStdString();
}

std::string EndlessReverb::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
