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

#include "limiter.hpp"
#include "../dsp/audio_context.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

Limiter::Limiter()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyThreshold().toStdString(), 1.0f, -2400, 0, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCeiling().toStdString(), 0.9f, -300, 0, -30, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRelease().toStdString(), 0.667f, 1, 1000, 667 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLookahead().toStdString(), 0.5f, 0, 10, 5 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyBoost().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });

    syncParameters();
}

void Limiter::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    updateCoefficients();
    applyLimiter(left, right);
}

void Limiter::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    updateCoefficients();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        applyLimiter(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Limiter::updateBuffers()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldUpdateBuffers || m_delayBufferL.empty()) {
        syncParameters();
        const uint32_t lookaheadSamples = static_cast<uint32_t>(m_lookaheadMs * m_sampleRate / 1000.0f);
        const uint32_t bufferSize = std::max(1u, lookaheadSamples + 1);
        if (bufferSize != m_delayBufferL.size()) {
            m_delayBufferL.assign(bufferSize, 0.0);
            m_delayBufferR.assign(bufferSize, 0.0);
            m_peakBuffer.assign(bufferSize, 0.0);
            m_maxIndices.clear();
            m_writePos = 0;
        }
        m_delaySamples = lookaheadSamples;
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldUpdateBuffers = false;
    }
}

void Limiter::updateCoefficients()
{
    if (m_sampleRate > 0) {
        m_releaseCoeff = std::exp(-1.0 / (static_cast<double>(m_releaseMs) * m_sampleRate / 1000.0));
    }
}

void Limiter::applyLimiter(double & left, double & right)
{
    if (m_delayBufferL.empty()) {
        updateBuffers();
    }

    const double thresholdLin = std::max(1.0e-6, static_cast<double>(Utils::Dsp::dbToLinear(m_thresholdDb)));
    const double ceilingLin = std::max(1.0e-6, static_cast<double>(Utils::Dsp::dbToLinear(m_ceilingDb)));

    // With boost the signal is limited to the threshold and lifted so that the threshold reaches the
    // ceiling (make-up = ceiling / threshold). Without boost it is pure peak limiting, capped at whichever
    // of threshold and ceiling is lower.
    const double targetLevel = m_boost ? thresholdLin : std::min(thresholdLin, ceilingLin);
    const double postGain = m_boost ? ceilingLin / thresholdLin : 1.0;

    const uint32_t size = static_cast<uint32_t>(m_delayBufferL.size());
    const double peak = std::max(std::abs(left), std::abs(right));

    // Maintain a sliding-window maximum of the peak over the whole lookahead buffer using a monotonic
    // decreasing deque. The slot at m_writePos currently holds the sample that is leaving the window.
    if (!m_maxIndices.empty() && m_maxIndices.front() == m_writePos) {
        m_maxIndices.pop_front();
    }
    while (!m_maxIndices.empty() && m_peakBuffer[m_maxIndices.back()] <= peak) {
        m_maxIndices.pop_back();
    }
    m_peakBuffer[m_writePos] = peak;
    m_delayBufferL[m_writePos] = left;
    m_delayBufferR[m_writePos] = right;
    m_maxIndices.push_back(m_writePos);

    // The gain is derived from the loudest sample within the lookahead window, so full reduction is reached
    // before the peak reaches the output (instant attack); the gain only eases back up via the release.
    const double windowMax = m_peakBuffer[m_maxIndices.front()];
    const double targetGain = windowMax > targetLevel ? targetLevel / windowMax : 1.0;
    if (targetGain < m_envGain) {
        m_envGain = targetGain;
    } else {
        m_envGain = m_releaseCoeff * m_envGain + (1.0 - m_releaseCoeff) * targetGain;
    }
    m_envGain = std::min(1.0, m_envGain);

    const uint32_t readPos = (m_writePos + 1) % size;
    double outL = m_delayBufferL[readPos] * m_envGain * postGain;
    double outR = m_delayBufferR[readPos] * m_envGain * postGain;

    m_writePos = readPos;

    // Brickwall backstop: guarantee the output never exceeds the ceiling.
    left = std::clamp(outL, -ceilingLin, ceilingLin);
    right = std::clamp(outR, -ceilingLin, ceilingLin);

    m_reductionDb = Utils::Dsp::linearToDb(static_cast<float>(m_envGain));

    // Denormal protection
    if (std::abs(m_reductionDb) < 1.0e-15) {
        m_reductionDb = 0.0;
    }
}

void Limiter::reset()
{
    m_envGain = 1.0;
    m_reductionDb = 0.0;
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0);
    std::fill(m_peakBuffer.begin(), m_peakBuffer.end(), 0.0);
    m_maxIndices.clear();
    m_writePos = 0;
}

void Limiter::sync()
{
    m_shouldUpdateBuffers = true;
}

float Limiter::reductionDb() const
{
    return static_cast<float>(m_reductionDb);
}

void Limiter::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        m_thresholdDb = -24.0f + p->get().value() * 24.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyCeiling().toStdString()); p) {
        m_ceilingDb = -3.0f + p->get().value() * 3.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        m_releaseMs = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 1.0, 1000.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLookahead().toStdString()); p) {
        m_lookaheadMs = p->get().value() * 10.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBoost().toStdString()); p) {
        m_boost = p->get().value() > 0.5f;
    }
}

std::string Limiter::typeIdString()
{
    return "3f8c1a2e-6b7d-4e9a-8f0c-2d3e4f5a6b7c";
}

std::string Limiter::type() const
{
    return Constants::RackEffectType::limiter().toStdString();
}

std::string Limiter::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
