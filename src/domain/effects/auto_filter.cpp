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

#include "auto_filter.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

AutoFilter::AutoFilter()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyFilterType().toStdString(), 0.0f, 0, 3, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFilterSlope().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyCutoff().toStdString(), 0.7f, 0, 10000, 7000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 10000, 3000, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoWaveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete }); // Triangle
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete }); // Normal
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoRate().toStdString(), 0.223f, 0, 10000, 2230, 100 }); // 1 Hz
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoIntensity().toStdString(), 0.815f, -10000, 10000, 6300, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Waveform().toStdString(), 1.0f, 0, 4, 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Mode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Rate().toStdString(), 0.223f, 0, 10000, 2230, 100 }); // 1 Hz
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfo2Intensity().toStdString(), 0.5f, -10000, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyStereoPhase().toStdString(), 0.0f, 0, 180, 0 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyEnvMod().toStdString(), 0.5f, -10000, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyAttack().toStdString(), 0.541f, 0, 1000, 541 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRelease().toStdString(), 0.697f, 0, 1000, 697 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100 });

    addMixParameter(1.0f, MixLaw::Crossfade);

    applyParameters();

    // So that a fresh effect and one that has been reset start from the same state, which the LFOs
    // would otherwise not do: a random shape draws its first value in reset() and nowhere else.
    AutoFilter::reset();
}

std::string AutoFilter::typeIdString()
{
    return "b7c8d9e0-f1a2-4b3c-9d4e-5f6a7b8c9d0e";
}

std::string AutoFilter::type() const
{
    return Constants::RackEffectType::autoFilter().toStdString();
}

std::string AutoFilter::typeId() const
{
    return typeIdString();
}

double AutoFilter::maxModulationOctaves()
{
    return 5.0;
}

double AutoFilter::maxResonance()
{
    return 0.97;
}

double AutoFilter::envelopeFloorDb()
{
    return -60.0;
}

void AutoFilter::applyParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFilterType().toStdString()); p) {
        const auto mode = [](int value) {
            switch (value) {
            case 1:
                return CascadedSvf::Mode::HighPass;
            case 2:
                return CascadedSvf::Mode::BandPass;
            case 3:
                return CascadedSvf::Mode::Notch;
            default:
                return CascadedSvf::Mode::LowPass;
            }
        }(p->get().xmlValue());
        m_isBandPass = mode == CascadedSvf::Mode::BandPass;
        m_filterL.setMode(mode);
        m_filterR.setMode(mode);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFilterSlope().toStdString()); p) {
        const int order = p->get().xmlValue() > 0 ? 4 : 2;
        m_filterStages = order / 2;
        m_filterL.setOrder(order);
        m_filterR.setOrder(order);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyCutoff().toStdString()); p) {
        m_cutoff = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyResonance().toStdString()); p) {
        m_resonance = static_cast<double>(p->get().value());
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoWaveform().toStdString()); p) {
        const auto waveform = static_cast<Lfo::Waveform>(p->get().xmlValue());
        m_cutoffLfoL.setWaveform(waveform);
        m_cutoffLfoR.setWaveform(waveform);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString()); p) {
        m_cutoffLfoMode = static_cast<Lfo::Mode>(p->get().xmlValue());
        m_cutoffLfoL.setMode(m_cutoffLfoMode);
        m_cutoffLfoR.setMode(m_cutoffLfoMode);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString()); p) {
        m_cutoffLfoRate = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString()); p) {
        m_cutoffLfoIntensity = ParameterMapper::mapCubicCentered((static_cast<double>(p->get().value()) - 0.5) * 2.0, -1.0, 1.0);
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Waveform().toStdString()); p) {
        const auto waveform = static_cast<Lfo::Waveform>(p->get().xmlValue());
        m_resonanceLfoL.setWaveform(waveform);
        m_resonanceLfoR.setWaveform(waveform);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Mode().toStdString()); p) {
        m_resonanceLfoMode = static_cast<Lfo::Mode>(p->get().xmlValue());
        m_resonanceLfoL.setMode(m_resonanceLfoMode);
        m_resonanceLfoR.setMode(m_resonanceLfoMode);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Rate().toStdString()); p) {
        m_resonanceLfoRate = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString()); p) {
        m_resonanceLfoIntensity = ParameterMapper::mapCubicCentered((static_cast<double>(p->get().value()) - 0.5) * 2.0, -1.0, 1.0);
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyStereoPhase().toStdString()); p) {
        // The parameter reads out in degrees, the LFO counts in cycles, and half a cycle is as far
        // apart as two channels can get before they start closing in on each other again.
        m_stereoPhase = static_cast<double>(p->get().value()) * 0.5;
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyEnvMod().toStdString()); p) {
        m_envIntensity = ParameterMapper::mapCubicCentered((static_cast<double>(p->get().value()) - 0.5) * 2.0, -1.0, 1.0);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        m_envAttackMs = ParameterMapper::mapExponential(static_cast<double>(p->get().value()), 0.1, 500.0);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRelease().toStdString()); p) {
        m_envReleaseMs = ParameterMapper::mapExponential(static_cast<double>(p->get().value()), 1.0, 2000.0);
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        const double gainDb = (static_cast<double>(p->get().value()) - 0.5) * 24.0;
        m_gain = std::pow(10.0, gainDb / 20.0);
    }

    updateEnvelopeCoefficients();
    updateLfoFrequencies();

    m_cutoffLfoR.setPhase(m_cutoffLfoL.phase() + m_stereoPhase);
    m_resonanceLfoR.setPhase(m_resonanceLfoL.phase() + m_stereoPhase);
}

double AutoFilter::bandPassCompensation(double resonance) const
{
    return std::pow(2.0 * (1.0 - resonance), static_cast<double>(m_filterStages));
}

void AutoFilter::updateLfoFrequencies()
{
    const auto frequency = [this](Lfo::Mode mode, double rate) {
        if (mode == Lfo::Mode::BPM) {
            return (static_cast<double>(bpm()) / 60.0) * (0.25 / std::max(0.0001, rate));
        }
        return ParameterMapper::mapLfoFrequency(rate, 0.05, 20.0);
    };

    const double cutoffFrequency = frequency(m_cutoffLfoMode, m_cutoffLfoRate);
    m_cutoffLfoL.setFrequency(cutoffFrequency);
    m_cutoffLfoR.setFrequency(cutoffFrequency);

    const double resonanceFrequency = frequency(m_resonanceLfoMode, m_resonanceLfoRate);
    m_resonanceLfoL.setFrequency(resonanceFrequency);
    m_resonanceLfoR.setFrequency(resonanceFrequency);
}

void AutoFilter::updateEnvelopeCoefficients()
{
    if (m_sampleRate <= 0) {
        return;
    }
    m_envAttackCoefficient = std::exp(-1.0 / (std::max(0.01, m_envAttackMs) * m_sampleRate / 1000.0));
    m_envReleaseCoefficient = std::exp(-1.0 / (std::max(0.01, m_envReleaseMs) * m_sampleRate / 1000.0));
}

void AutoFilter::updateSampleRateDependents()
{
    const double maxFrequency = std::min(20000.0, m_sampleRate * 0.49);
    m_octaveSpan = std::log2(maxFrequency / 20.0);

    m_filterL.setSampleRate(m_sampleRate);
    m_filterR.setSampleRate(m_sampleRate);

    // The LFOs are read once per control-rate step, so that is the rate they run at.
    const double controlRate = m_sampleRate / controlRateStride;
    m_cutoffLfoL.setSampleRate(controlRate);
    m_cutoffLfoR.setSampleRate(controlRate);
    m_resonanceLfoL.setSampleRate(controlRate);
    m_resonanceLfoR.setSampleRate(controlRate);

    updateEnvelopeCoefficients();
    updateLfoFrequencies();
}

void AutoFilter::updateEnvelope(double left, double right)
{
    const double level = std::max(std::abs(left), std::abs(right));
    const double targetDb = std::max(level > 1e-9 ? 20.0 * std::log10(level) : envelopeFloorDb(), envelopeFloorDb());
    const double coefficient = targetDb > m_envelopeDb ? m_envAttackCoefficient : m_envReleaseCoefficient;
    m_envelopeDb = coefficient * m_envelopeDb + (1.0 - coefficient) * targetDb;
}

void AutoFilter::updateModulation()
{
    const double envelope = std::clamp((m_envelopeDb - envelopeFloorDb()) / -envelopeFloorDb(), 0.0, 1.0);

    // The filter's own cutoff is exponential in its 0 to 1 domain, so an offset there is already an
    // offset in octaves and only has to be scaled by how many octaves that domain covers.
    const double octavesToNormalized = maxModulationOctaves() / std::max(m_octaveSpan, 0.0001);
    const double envelopeModulation = envelope * m_envIntensity;

    const double cutoffL = m_cutoff + (m_cutoffLfoL.nextSample() * m_cutoffLfoIntensity + envelopeModulation) * octavesToNormalized;
    const double cutoffR = m_cutoff + (m_cutoffLfoR.nextSample() * m_cutoffLfoIntensity + envelopeModulation) * octavesToNormalized;
    m_filterL.setCutoff(std::clamp(cutoffL, 0.0, 1.0));
    m_filterR.setCutoff(std::clamp(cutoffR, 0.0, 1.0));

    const double resonanceL = std::clamp(m_resonance + m_resonanceLfoL.nextSample() * m_resonanceLfoIntensity, 0.0, maxResonance());
    const double resonanceR = std::clamp(m_resonance + m_resonanceLfoR.nextSample() * m_resonanceLfoIntensity, 0.0, maxResonance());
    m_filterL.setResonance(resonanceL);
    m_filterR.setResonance(resonanceR);

    m_outputGainL = m_isBandPass ? m_gain * bandPassCompensation(resonanceL) : m_gain;
    m_outputGainR = m_isBandPass ? m_gain * bandPassCompensation(resonanceR) : m_gain;
}

void AutoFilter::processSample(double & left, double & right)
{
    if (m_sampleRate <= 0) {
        return;
    }

    if (std::abs(m_sampleRate - m_appliedSampleRate) > 0.1) {
        m_appliedSampleRate = m_sampleRate;
        updateSampleRateDependents();
    }

    if (m_shouldApplyParameters) {
        m_shouldApplyParameters = false;
        applyParameters();
    }

    updateEnvelope(left, right);

    if (!m_controlCounter) {
        updateModulation();
    }
    if (++m_controlCounter >= controlRateStride) {
        m_controlCounter = 0;
    }

    left = m_filterL.process(left) * m_outputGainL;
    right = m_filterR.process(right) * m_outputGainR;
}

void AutoFilter::sync()
{
    // Applied on the audio thread, because that is the only thread the filters and the LFOs are
    // touched from.
    m_shouldApplyParameters = true;
}

void AutoFilter::setBpm(float bpm)
{
    Effect::setBpm(bpm);
    m_shouldApplyParameters = true;
}

void AutoFilter::reset()
{
    m_filterL.reset();
    m_filterR.reset();
    m_cutoffLfoL.reset();
    m_cutoffLfoR.reset();
    m_resonanceLfoL.reset();
    m_resonanceLfoR.reset();
    m_envelopeDb = envelopeFloorDb();
    m_controlCounter = 0;
    // Resetting the LFOs put them all back to phase zero, so the stereo offset has to be laid on
    // again before the first sample of the next run.
    m_shouldApplyParameters = true;
}

} // namespace noteahead
