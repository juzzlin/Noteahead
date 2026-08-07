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

#include "auto_ducker.hpp"
#include "../dsp/audio_context.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

AutoDucker::AutoDucker()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyThreshold().toStdString(), 0.66667f, -6000, 0, -2000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAmount().toStdString(), 0.25f, -2400, 2400, -1200, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyKnee().toStdString(), 0.25f, 0, 2400, 600, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.45f, 0, 1000, 450 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRelease().toStdString(), 0.7f, 0, 1000, 700 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyHold().toStdString(), 0.0f, 0, 500, 0 });
    addParameter(Parameter { Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString(), -1.0f, -1, static_cast<int>(Constants::deviceRackSize()) - 1, -1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySideChainLpf().toStdString(), 1.0f, 0, 1000, 1000, 1 });

    m_sideChainLpfL.setMode(CascadedSvf::Mode::LowPass);
    m_sideChainLpfR.setMode(CascadedSvf::Mode::LowPass);

    syncParameters();
    updateCoefficients();
}

std::optional<size_t> AutoDucker::sidechainSourceDeviceIndex() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        if (const int val = p->get().xmlValue(); val >= 0) {
            return static_cast<size_t>(val);
        }
    }
    return std::nullopt;
}

void AutoDucker::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateState();

    // No context here, so the effect's own input is the only detector available
    updateEnvelope(targetGainDb(detectorLevelDb(left, right)));
    applyGain(left, right);
}

void AutoDucker::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateState();

    const bool hasSidechain = m_sidechainSourceDevice && *m_sidechainSourceDevice < context.deviceOutputBuffers.size();
    const auto sidechainBuffer = hasSidechain ? context.deviceOutputBuffers[*m_sidechainSourceDevice] : std::span<const double> {};

    for (uint32_t i = 0; i < context.frameCount; i++) {
        double detectorL = context.buffer[i * 2];
        double detectorR = context.buffer[i * 2 + 1];

        if (hasSidechain && !sidechainBuffer.empty()) {
            detectorL = sidechainBuffer[i * 2];
            detectorR = sidechainBuffer[i * 2 + 1];
        }

        updateEnvelope(targetGainDb(detectorLevelDb(detectorL, detectorR)));
        applyGain(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void AutoDucker::updateState()
{
    if (static_cast<uint32_t>(m_sampleRate) != m_lastSampleRate || m_shouldSyncParameters) {
        syncParameters();
        updateCoefficients();
        m_lastSampleRate = static_cast<uint32_t>(m_sampleRate);
        m_shouldSyncParameters = false;
    }
}

void AutoDucker::updateCoefficients()
{
    if (m_sampleRate > 0) {
        m_attackCoeff = std::exp(-1.0 / (static_cast<double>(m_attackMs) * m_sampleRate / 1000.0));
        m_releaseCoeff = std::exp(-1.0 / (static_cast<double>(m_releaseMs) * m_sampleRate / 1000.0));
        m_holdSamples = static_cast<uint32_t>(static_cast<double>(m_holdMs) * m_sampleRate / 1000.0);
    }
}

double AutoDucker::detectorLevelDb(double left, double right)
{
    if (m_sideChainLpfCutoff < 1.0f) {
        left = m_sideChainLpfL.process(left);
        right = m_sideChainLpfR.process(right);
    }

    return Utils::Dsp::linearToDb(static_cast<float>(std::max(std::abs(left), std::abs(right))));
}

double AutoDucker::targetGainDb(double detectorDb) const
{
    const double threshold = static_cast<double>(m_threshold);
    const double knee = static_cast<double>(m_knee);

    // Amount is reached once the detector is a full knee above the threshold. A zero knee makes
    // that a switch at the threshold, which is what a gate-style duck wants
    double engagement = 0.0;
    if (knee > 0.001) {
        engagement = std::clamp((detectorDb - threshold + knee / 2.0) / knee, 0.0, 1.0);
    } else {
        engagement = detectorDb > threshold ? 1.0 : 0.0;
    }

    return static_cast<double>(m_amount) * engagement;
}

void AutoDucker::updateEnvelope(double targetDb)
{
    if (std::abs(targetDb) > std::abs(m_envelopeDb)) {
        // Engaging, so follow at the attack rate and keep the hold timer charged behind it
        m_envelopeDb = m_attackCoeff * m_envelopeDb + (1.0 - m_attackCoeff) * targetDb;
        m_holdCounter = m_holdSamples;
    } else if (m_holdCounter > 0) {
        m_holdCounter--;
    } else {
        m_envelopeDb = m_releaseCoeff * m_envelopeDb + (1.0 - m_releaseCoeff) * targetDb;
    }

    // Denormal protection
    if (std::abs(m_envelopeDb) < 1.0e-15) {
        m_envelopeDb = 0.0;
    }
}

void AutoDucker::applyGain(double & left, double & right) const
{
    const double gain = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(m_envelopeDb)));
    left *= gain;
    right *= gain;
}

void AutoDucker::reset()
{
    m_envelopeDb = 0.0;
    m_holdCounter = 0;
    m_sideChainLpfL.reset();
    m_sideChainLpfR.reset();
}

void AutoDucker::sync()
{
    m_shouldSyncParameters = true;
}

float AutoDucker::gainDb() const
{
    return static_cast<float>(m_envelopeDb);
}

void AutoDucker::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        m_threshold = -60.0f + p->get().value() * 60.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); p) {
        m_amount = -24.0f + p->get().value() * 48.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyKnee().toStdString()); p) {
        m_knee = p->get().value() * 24.0f;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        m_attackMs = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 0.1, 500.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        m_releaseMs = static_cast<float>(ParameterMapper::mapExponential(p->get().value(), 1.0, 2000.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyHold().toStdString()); p) {
        m_holdMs = p->get().value() * 500.0f;
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
        m_sideChainLpfL.setSampleRate(m_sampleRate > 0 ? m_sampleRate : Constants::defaultSampleRate());
        m_sideChainLpfR.setSampleRate(m_sampleRate > 0 ? m_sampleRate : Constants::defaultSampleRate());
        m_sideChainLpfL.setCutoff(m_sideChainLpfCutoff);
        m_sideChainLpfR.setCutoff(m_sideChainLpfCutoff);
    }
}

std::string AutoDucker::typeIdString()
{
    return "8c1d5e93-4f27-4a6b-b0d8-3e7a1c9f5b24";
}

std::string AutoDucker::type() const
{
    return Constants::RackEffectType::autoDucker().toStdString();
}

std::string AutoDucker::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
