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

#include "common/constants.hpp"
#include "common/parameter_mapper.hpp"
#include "common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

CompressorEffect::CompressorEffect()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyThreshold().toStdString(), 0.66f, -6000, 0, -2000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRatio().toStdString(), 0.15789f, 100, 2000, 400, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.2f, 0, 500, 10 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRelease().toStdString(), 0.25f, 1, 2000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyKnee().toStdString(), 0.0f, 0, 2400, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyMakeup().toStdString(), 0.5f, -1200, 1200, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLookahead().toStdString(), 0.0f, 0, 10, 0, 1, Parameter::Type::Continuous, { "Lookahead" } });
    addParameter(Parameter { Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString(), -1.0f, -1, static_cast<int>(Constants::deviceRackSize()) - 1, -1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySideChainLpf().toStdString(), 1.0f, 0, 1000, 1000, 1 });

    m_sideChainLpfL.setMode(CascadedSvf::Mode::LowPass);
    m_sideChainLpfR.setMode(CascadedSvf::Mode::LowPass);

    syncParameters();
}

std::optional<size_t> CompressorEffect::sidechainSourceDeviceIndex() const
{
    return m_sidechainSourceDevice;
}

void CompressorEffect::process(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();
    updateCoefficients();

    const double detectorDb = calculateDetectorLevelDb(left, right);
    const double gainReductionDb = calculateGainReductionDb(detectorDb);

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

    const bool hasSidechain = m_sidechainSourceDevice && *m_sidechainSourceDevice < context.deviceOutputBuffers.size();
    const auto sidechainBuffer = hasSidechain ? context.deviceOutputBuffers[*m_sidechainSourceDevice] : std::span<const double> {};

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double detectorL = context.buffer[i * 2];
        double detectorR = context.buffer[i * 2 + 1];

        if (hasSidechain && !sidechainBuffer.empty()) {
            detectorL = sidechainBuffer[i * 2];
            detectorR = sidechainBuffer[i * 2 + 1];
        }

        if (m_sideChainLpfCutoff < 1.0f) {
            detectorL = m_sideChainLpfL.process(detectorL);
            detectorR = m_sideChainLpfR.process(detectorR);
        }

        const double detectorDb = calculateDetectorLevelDb(detectorL, detectorR);
        const double gainReductionDb = calculateGainReductionDb(detectorDb);

        updateEnvelope(gainReductionDb);
        applyGain(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void CompressorEffect::updateBuffers()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldUpdateBuffers || m_delayBufferL.empty()) {
        syncParameters();
        const uint32_t lookaheadSamples = static_cast<uint32_t>(m_lookaheadMs * m_sampleRate / 1000.0f);
        const uint32_t bufferSize = std::max(1u, lookaheadSamples + 1);
        if (bufferSize != m_delayBufferL.size()) {
            m_delayBufferL.assign(bufferSize, 0.0);
            m_delayBufferR.assign(bufferSize, 0.0);
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
        m_attackCoeff = std::exp(-1.0 / (static_cast<double>(m_attackMs) * m_sampleRate / 1000.0));
        m_releaseCoeff = std::exp(-1.0 / (static_cast<double>(m_releaseMs) * m_sampleRate / 1000.0));
    }
}

double CompressorEffect::calculateDetectorLevelDb(double left, double right) const
{
    const double detector = std::max(std::abs(left), std::abs(right));
    return Utils::Dsp::linearToDb(static_cast<float>(detector)); // Assuming linearToDb can handle float
}

double CompressorEffect::calculateGainReductionDb(double detectorDb) const
{
    double targetDb = detectorDb;
    const double threshold = static_cast<double>(m_threshold);
    const double knee = static_cast<double>(m_knee);
    const double ratio = static_cast<double>(m_ratio);

    if (knee > 0.001) {
        if (detectorDb > threshold + knee / 2.0) {
            targetDb = threshold + (detectorDb - threshold) / ratio;
        } else if (detectorDb > threshold - knee / 2.0) {
            const double diff = detectorDb - threshold + knee / 2.0;
            targetDb = detectorDb + (1.0 / ratio - 1.0) * diff * diff / (2.0 * knee);
        }
    } else {
        if (detectorDb > threshold) {
            targetDb = threshold + (detectorDb - threshold) / ratio;
        }
    }

    return targetDb - detectorDb;
}

void CompressorEffect::updateEnvelope(double gainReductionDb)
{
    if (gainReductionDb < m_envelopeDb) {
        m_envelopeDb = m_attackCoeff * m_envelopeDb + (1.0 - m_attackCoeff) * gainReductionDb;
    } else {
        m_envelopeDb = m_releaseCoeff * m_envelopeDb + (1.0 - m_releaseCoeff) * gainReductionDb;
    }

    // Denormal protection
    if (std::abs(m_envelopeDb) < 1.0e-15) {
        m_envelopeDb = 0.0;
    }

    m_reductionDb = m_envelopeDb;
}

void CompressorEffect::applyGain(double & left, double & right)
{
    if (m_delayBufferL.empty()) {
        updateBuffers();
    }

    m_delayBufferL[m_writePos] = left;
    m_delayBufferR[m_writePos] = right;

    const uint32_t readPos = (m_writePos + m_delayBufferL.size() - m_delaySamples) % m_delayBufferL.size();
    double outL = m_delayBufferL[readPos];
    double outR = m_delayBufferR[readPos];

    m_writePos = (m_writePos + 1) % m_delayBufferL.size();

    const double totalGainDb = m_envelopeDb + static_cast<double>(m_makeup);
    const double totalGain = Utils::Dsp::dbToLinear(static_cast<float>(totalGainDb));

    left = outL * totalGain;
    right = outR * totalGain;
}

void CompressorEffect::reset()
{
    m_envelopeDb = 0.0;
    m_reductionDb = 0.0;
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0);
    m_writePos = 0;
    m_sideChainLpfL.reset();
    m_sideChainLpfR.reset();
}

void CompressorEffect::sync()
{
    m_shouldUpdateBuffers = true;
}

float CompressorEffect::reductionDb() const
{
    return static_cast<float>(m_reductionDb);
}

void CompressorEffect::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        m_threshold = -60.0f + p->get().value() * 60.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRatio().toStdString()); p) {
        m_ratio = 1.0f + p->get().value() * 19.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        m_attackMs = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 0.1, 500.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        m_releaseMs = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 1.0, 2000.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyKnee().toStdString()); p) {
        m_knee = p->get().value() * 24.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMakeup().toStdString()); p) {
        m_makeup = -12.0f + p->get().value() * 24.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLookahead().toStdString()); p) {
        m_lookaheadMs = p->get().value() * 10.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        if (const int val = p->get().xmlValue(); val >= 0) {
            m_sidechainSourceDevice = static_cast<size_t>(val);
        } else {
            m_sidechainSourceDevice = std::nullopt;
        }
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainLpf().toStdString()); p) {
        m_sideChainLpfCutoff = p->get().value();
        m_sideChainLpfL.setSampleRate(m_sampleRate > 0 ? m_sampleRate : 48000.0);
        m_sideChainLpfR.setSampleRate(m_sampleRate > 0 ? m_sampleRate : 48000.0);
        m_sideChainLpfL.setCutoff(m_sideChainLpfCutoff);
        m_sideChainLpfR.setCutoff(m_sideChainLpfCutoff);
    }
}

std::string CompressorEffect::typeIdString()
{
    return "7a2b3c4d-5e6f-4a8b-9c0d-1e2f3a4b5c6d";
}

std::string CompressorEffect::type() const
{
    return Constants::RackEffectType::compressor().toStdString();
}

std::string CompressorEffect::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
