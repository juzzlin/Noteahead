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

#include "compressor.hpp"
#include "../dsp/audio_context.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"

#include <algorithm>

namespace noteahead {

Compressor::Compressor()
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
    addParameter(Parameter { Constants::NahdXml::xmlKeyMode().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });

    m_sideChainLpfL.setMode(CascadedSvf::Mode::LowPass);
    m_sideChainLpfR.setMode(CascadedSvf::Mode::LowPass);

    syncParameters();
}

std::optional<size_t> Compressor::sidechainSourceDeviceIndex() const
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        if (const int val = p->get().xmlValue(); val >= 0) {
            return static_cast<size_t>(val);
        }
    }
    return std::nullopt;
}

void Compressor::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();

    m_core.processGainDb(left, right);
    applyGain(left, right);
}

void Compressor::processBlock(AudioContext & context)
{
    if (m_sampleRate <= 0) {
        return;
    }

    updateBuffers();

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

        m_core.processGainDb(detectorL, detectorR);
        applyGain(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Compressor::updateBuffers()
{
    m_core.setSampleRate(m_sampleRate);

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

void Compressor::applyGain(double & left, double & right)
{
    if (m_delayBufferL.empty()) {
        updateBuffers();
    }

    m_delayBufferL[m_writePos] = left;
    m_delayBufferR[m_writePos] = right;

    const uint32_t readPos = (m_writePos + m_delayBufferL.size() - m_delaySamples) % m_delayBufferL.size();
    const double outL = m_delayBufferL[readPos];
    const double outR = m_delayBufferR[readPos];

    m_writePos = (m_writePos + 1) % m_delayBufferL.size();

    const double totalGainDb = m_core.reductionDb() + static_cast<double>(m_makeup);
    const double totalGain = Utils::Dsp::dbToLinear(static_cast<float>(totalGainDb));

    left = outL * totalGain;
    right = outR * totalGain;
}

void Compressor::reset()
{
    m_core.reset();
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0);
    m_writePos = 0;
    m_sideChainLpfL.reset();
    m_sideChainLpfR.reset();
}

void Compressor::sync()
{
    m_shouldUpdateBuffers = true;
}

float Compressor::reductionDb() const
{
    return static_cast<float>(m_core.reductionDb());
}

void Compressor::syncParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        m_core.setThresholdDb(-60.0 + static_cast<double>(p->get().value()) * 60.0);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRatio().toStdString()); p) {
        m_core.setRatio(1.0 + static_cast<double>(p->get().value()) * 19.0);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        m_core.setAttackMs(ParameterMapper::mapExponential(p->get().value(), 0.1, 500.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        m_core.setReleaseMs(ParameterMapper::mapExponential(p->get().value(), 1.0, 2000.0));
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyKnee().toStdString()); p) {
        m_core.setKneeDb(static_cast<double>(p->get().value()) * 24.0);
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
    if (const auto p = parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        m_core.setDetectorMode(p->get().value() > 0.5f ? DetectorMode::Rms : DetectorMode::Peak);
    }
}

namespace {

//! A preset in the units it is reasoned about, rather than in the normalized 0-1 the parameters
//! store. applyPreset() does the conversion, so the table below stays readable and reviewable.
struct PresetValues
{
    float thresholdDb;
    float ratio;
    float attackMs;
    float releaseMs;
    float kneeDb;
    float makeupDb;
    float lookaheadMs;
    bool rms;
};

PresetValues presetValues(Compressor::Preset preset)
{
    switch (preset) {
    // Mix-bus glue. The release is short for a bus on purpose: at 140 BPM a four-on-the-floor kick
    // lands every 428 ms, and a release of a few hundred milliseconds is still recovering when the
    // next one arrives, which takes the front off it.
    case Compressor::Preset::Glue:
        return { -22.0f, 2.0f, 30.0f, 120.0f, 6.0f, 2.0f, 0.0f, true };
    case Compressor::Preset::DrumBus:
        return { -20.0f, 4.0f, 10.0f, 100.0f, 3.0f, 3.0f, 0.0f, false };
    // The slow attack is the point: it lets the stick through before the body is caught.
    case Compressor::Preset::PunchyDrums:
        return { -22.0f, 4.0f, 30.0f, 80.0f, 0.0f, 4.0f, 0.0f, false };
    case Compressor::Preset::Piano:
        return { -22.0f, 3.0f, 20.0f, 150.0f, 6.0f, 3.0f, 0.0f, true };
    case Compressor::Preset::Vocal:
        return { -20.0f, 3.0f, 5.0f, 150.0f, 6.0f, 4.0f, 0.0f, true };
    // Threshold well above the others: the bass devices run some 10 dB hotter than the rest.
    case Compressor::Preset::Bass:
        return { -16.0f, 4.0f, 15.0f, 100.0f, 3.0f, 4.0f, 0.0f, false };
    case Compressor::Preset::Pump:
        return { -24.0f, 8.0f, 0.5f, 180.0f, 0.0f, 5.0f, 0.0f, false };
    // Lookahead so the ceiling holds on transients the attack alone would miss.
    case Compressor::Preset::Brickwall:
        return { -6.0f, 20.0f, 0.1f, 50.0f, 0.0f, 0.0f, 5.0f, false };
    }
    return { -22.0f, 2.0f, 30.0f, 120.0f, 6.0f, 2.0f, 0.0f, true };
}

} // namespace

void Compressor::applyPreset(Preset preset)
{
    const auto values = presetValues(preset);

    // The inverses of the mappings in syncParameters().
    const auto set = [this](const QString & key, float value) {
        if (const auto p = parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };

    set(Constants::NahdXml::xmlKeyThreshold(), (values.thresholdDb + 60.0f) / 60.0f);
    set(Constants::NahdXml::xmlKeyRatio(), (values.ratio - 1.0f) / 19.0f);
    set(Constants::NahdXml::xmlKeyAttack(), static_cast<float>(ParameterMapper::unmapExponential(values.attackMs, 0.1, 500.0)));
    set(Constants::NahdXml::xmlKeyRelease(), static_cast<float>(ParameterMapper::unmapExponential(values.releaseMs, 1.0, 2000.0)));
    set(Constants::NahdXml::xmlKeyKnee(), values.kneeDb / 24.0f);
    set(Constants::NahdXml::xmlKeyMakeup(), (values.makeupDb + 12.0f) / 24.0f);
    set(Constants::NahdXml::xmlKeyLookahead(), values.lookaheadMs / 10.0f);
    set(Constants::NahdXml::xmlKeyMode(), values.rms ? 1.0f : 0.0f);

    m_shouldSyncParameters = true;
    m_shouldUpdateBuffers = true;
}

std::string Compressor::presetToString(Preset preset)
{
    switch (preset) {
    case Preset::Glue:
        return "Glue";
    case Preset::DrumBus:
        return "Drum Bus";
    case Preset::PunchyDrums:
        return "Punchy Drums";
    case Preset::Piano:
        return "Piano";
    case Preset::Vocal:
        return "Vocal";
    case Preset::Bass:
        return "Bass";
    case Preset::Pump:
        return "Pump";
    case Preset::Brickwall:
        return "Brickwall";
    }
    return "Glue";
}

Compressor::Preset Compressor::stringToPreset(const std::string & presetName)
{
    if (presetName == "Drum Bus")
        return Preset::DrumBus;
    if (presetName == "Punchy Drums")
        return Preset::PunchyDrums;
    if (presetName == "Piano")
        return Preset::Piano;
    if (presetName == "Vocal")
        return Preset::Vocal;
    if (presetName == "Bass")
        return Preset::Bass;
    if (presetName == "Pump")
        return Preset::Pump;
    if (presetName == "Brickwall")
        return Preset::Brickwall;
    return Preset::Glue;
}

std::vector<std::string> Compressor::presetNames()
{
    return { "Glue", "Drum Bus", "Punchy Drums", "Piano", "Vocal", "Bass", "Pump", "Brickwall" };
}

std::string Compressor::typeIdString()
{
    return "7a2b3c4d-5e6f-4a8b-9c0d-1e2f3a4b5c6d";
}

std::string Compressor::type() const
{
    return Constants::RackEffectType::compressor().toStdString();
}

std::string Compressor::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
