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

#include "compressor_effect.hpp"
#include "audio_context.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

CompressorEffect::CompressorEffect()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyCompressorThreshold().toStdString(), 0.66f, -60, 0, -20 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCompressorRatio().toStdString(), 0.15f, 1, 20, 4 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.2f, 0, 500, 10 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRelease().toStdString(), 0.25f, 1, 2000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCompressorKnee().toStdString(), 0.0f, 0, 24, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCompressorMakeup().toStdString(), 0.5f, -12, 12, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLookahead().toStdString(), 0.0f, 0, 10, 0 });
    
    syncParameters();
}

void CompressorEffect::process(float & left, float & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    updateCoefficients();

    const float detectorDb = calculateDetectorLevelDb(left, right);
    const float gainReductionDb = calculateGainReductionDb(detectorDb);

    updateEnvelope(gainReductionDb);
    applyGain(left, right);
}

void CompressorEffect::process(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    updateCoefficients();

    for (uint32_t i = 0; i < context.frameCount; i++) {
        process(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void CompressorEffect::updateBuffers()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldUpdateBuffers || m_delayBufferL.empty()) {
        syncParameters();
        const uint32_t lookaheadSamples = static_cast<uint32_t>(m_lookaheadMs * m_sampleRate / 1000.0f);
        const uint32_t bufferSize = std::max(1u, lookaheadSamples + 1);
        if (bufferSize != m_delayBufferL.size()) {
            m_delayBufferL.assign(bufferSize, 0.0f);
            m_delayBufferR.assign(bufferSize, 0.0f);
            m_writePos = 0;
        }
        m_delaySamples = lookaheadSamples;
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldUpdateBuffers = false;
        m_shouldSyncParameters = false;
    } else if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
}

void CompressorEffect::updateCoefficients()
{
    if (m_sampleRate > 0) {
        m_attackCoeff = std::exp(-1.0f / (m_attackMs * static_cast<float>(m_sampleRate) / 1000.0f));
        m_releaseCoeff = std::exp(-1.0f / (m_releaseMs * static_cast<float>(m_sampleRate) / 1000.0f));
    }
}

float CompressorEffect::calculateDetectorLevelDb(float left, float right) const
{
    const float detector = std::max(std::abs(left), std::abs(right));
    return Utils::Dsp::linearToDb(detector);
}

float CompressorEffect::calculateGainReductionDb(float detectorDb) const
{
    float targetDb = detectorDb;
    if (m_knee > 0.001f) {
        if (detectorDb > m_threshold + m_knee / 2.0f) {
            targetDb = m_threshold + (detectorDb - m_threshold) / m_ratio;
        } else if (detectorDb > m_threshold - m_knee / 2.0f) {
            const float diff = detectorDb - m_threshold + m_knee / 2.0f;
            targetDb = detectorDb + (1.0f / m_ratio - 1.0f) * diff * diff / (2.0f * m_knee);
        }
    } else {
        if (detectorDb > m_threshold) {
            targetDb = m_threshold + (detectorDb - m_threshold) / m_ratio;
        }
    }

    return targetDb - detectorDb;
}

void CompressorEffect::updateEnvelope(float gainReductionDb)
{
    if (gainReductionDb < m_envelopeDb) {
        m_envelopeDb = m_attackCoeff * m_envelopeDb + (1.0f - m_attackCoeff) * gainReductionDb;
    } else {
        m_envelopeDb = m_releaseCoeff * m_envelopeDb + (1.0f - m_releaseCoeff) * gainReductionDb;
    }

    // Denormal protection
    if (std::abs(m_envelopeDb) < 1.0e-15f) {
        m_envelopeDb = 0.0f;
    }

    m_reductionDb = m_envelopeDb;
}

void CompressorEffect::applyGain(float & left, float & right)
{
    if (m_delayBufferL.empty()) {
        updateBuffers();
    }
    
    m_delayBufferL[m_writePos] = left;
    m_delayBufferR[m_writePos] = right;

    const uint32_t readPos = (m_writePos + m_delayBufferL.size() - m_delaySamples) % m_delayBufferL.size();
    float outL = m_delayBufferL[readPos];
    float outR = m_delayBufferR[readPos];

    m_writePos = (m_writePos + 1) % m_delayBufferL.size();

    const float totalGainDb = m_envelopeDb + m_makeup;
    const float totalGain = Utils::Dsp::dbToLinear(totalGainDb);

    left = outL * totalGain;
    right = outR * totalGain;
}

void CompressorEffect::reset()
{
    m_envelopeDb = 0.0f;
    m_reductionDb = 0.0f;
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0f);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0f);
    m_writePos = 0;
}

void CompressorEffect::sync()
{
    m_shouldUpdateBuffers = true;
}

float CompressorEffect::reductionDb() const
{
    return m_reductionDb;
}

void CompressorEffect::syncParameters()
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyCompressorThreshold().toStdString()); p) {
        m_threshold = -60.0f + p->get().value() * 60.0f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyCompressorRatio().toStdString()); p) {
        m_ratio = 1.0f + p->get().value() * 19.0f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        m_attackMs = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 0.1, 500.0));
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        m_releaseMs = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 1.0, 2000.0));
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyCompressorKnee().toStdString()); p) {
        m_knee = p->get().value() * 24.0f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyCompressorMakeup().toStdString()); p) {
        m_makeup = -12.0f + p->get().value() * 24.0f;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyLookahead().toStdString()); p) {
        m_lookaheadMs = p->get().value() * 10.0f;
    }
}

} // namespace noteahead
