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

#include "phaser.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Range the centre of the sweep can be placed in. Below this the notches sit under most of the
//! signal and the effect stops being audible; above it there is little left to cancel.
constexpr double minCentreFrequency = 50.0;
constexpr double maxCentreFrequency = 8000.0;

} // namespace

Phaser::Phaser()
{
    // Even stage counts only: an all-pass pair is what makes one notch, and an odd section left over
    // shifts the phase without cancelling anything.
    addParameter(Parameter { Constants::NahdXml::xmlKeyStages().toStdString(), 6.0f, 2, maxStages(), 6, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFrequency().toStdString(), 0.53f, 0, 10000, 5300, 100 }); // ~700 Hz
    addParameter(Parameter { Constants::NahdXml::xmlKeyDepth().toStdString(), 0.7f, 0, 10000, 7000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyFeedback().toStdString(), 0.5f, -10000, 10000, 0, 100 });

    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoWaveform().toStdString(), 3.0f, 0, 4, 3, 1, Parameter::Type::Discrete }); // Sine
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete }); // Normal
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoRate().toStdString(), 0.223f, 0, 10000, 2230, 100 }); // 1 Hz
    addParameter(Parameter { Constants::NahdXml::xmlKeyRateDivider().toStdString(), 1.0f, 1, maxRateDivider(), 1, 1, Parameter::Type::Discrete });

    addParameter(Parameter { Constants::NahdXml::xmlKeyStereoPhase().toStdString(), 0.5f, 0, 180, 90 }); // Quadrature
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -1200, 1200, 0, 100 });

    // The notches are deepest where the dry and the wet meet at equal amounts, which is where a
    // phaser is normally run and so where it starts.
    addMixParameter(0.5f, MixLaw::Crossfade);

    applyParameters();
    Phaser::reset();
}

std::string Phaser::typeIdString()
{
    return "c3d4e5f6-a7b8-4c9d-0e1f-2a3b4c5d6e7f";
}

std::string Phaser::type() const
{
    return Constants::RackEffectType::phaser().toStdString();
}

std::string Phaser::typeId() const
{
    return typeIdString();
}

int Phaser::maxStages()
{
    return 12;
}

double Phaser::maxSweepOctaves()
{
    return 3.0;
}

double Phaser::maxFeedback()
{
    return 0.95;
}

int Phaser::maxRateDivider()
{
    return 64;
}

void Phaser::applyParameters()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStages().toStdString()); p) {
        // Rounded down to an even count rather than clamped, so the knob reads what it does.
        m_stages = std::clamp(p->get().xmlValue() / 2 * 2, 2, maxStages());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFrequency().toStdString()); p) {
        m_centreFrequency = ParameterMapper::mapExponential(static_cast<double>(p->get().value()), minCentreFrequency, maxCentreFrequency);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDepth().toStdString()); p) {
        m_depth = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFeedback().toStdString()); p) {
        m_feedback = ParameterMapper::mapCubicCentered((static_cast<double>(p->get().value()) - 0.5) * 2.0, -maxFeedback(), maxFeedback());
    }

    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoWaveform().toStdString()); p) {
        const auto waveform = static_cast<Lfo::Waveform>(p->get().xmlValue());
        m_lfoLeft.setWaveform(waveform);
        m_lfoRight.setWaveform(waveform);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString()); p) {
        m_lfoMode = static_cast<Lfo::Mode>(p->get().xmlValue());
        m_lfoLeft.setMode(m_lfoMode);
        m_lfoRight.setMode(m_lfoMode);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString()); p) {
        m_rate = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString()); p) {
        m_rateDivider = std::clamp(p->get().xmlValue(), 1, maxRateDivider());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStereoPhase().toStdString()); p) {
        m_stereoPhase = static_cast<double>(p->get().value()) * 0.5;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_gain = std::pow(10.0, (static_cast<double>(p->get().value()) - 0.5) * 24.0 / 20.0);
    }

    updateLfoFrequency();

    m_lfoRight.setPhase(m_lfoLeft.phase() + m_stereoPhase);
}

void Phaser::updateLfoFrequency()
{
    // The divider sits after both, so a tempo-locked sweep can be stretched over several bars just
    // as a free-running one can be stretched over minutes.
    const double frequency = (m_lfoMode == Lfo::Mode::BPM
                                ? (static_cast<double>(bpm()) / 60.0) * (0.25 / std::max(0.0001, m_rate))
                                : ParameterMapper::mapLfoFrequency(m_rate, 0.05, 20.0))
      / m_rateDivider;
    m_lfoLeft.setFrequency(frequency);
    m_lfoRight.setFrequency(frequency);
}

void Phaser::updateSampleRateDependents()
{
    // The LFOs are read once per control-rate step, so that is the rate they run at.
    const double controlRate = m_sampleRate / controlRateStride;
    m_lfoLeft.setSampleRate(controlRate);
    m_lfoRight.setSampleRate(controlRate);
    updateLfoFrequency();
}

double Phaser::coefficientForFrequency(double frequency) const
{
    const double corner = std::clamp(frequency, 20.0, m_sampleRate * 0.49);
    const double tangent = std::tan(std::numbers::pi * corner / m_sampleRate);
    return (tangent - 1.0) / (tangent + 1.0);
}

void Phaser::updateModulation()
{
    const double octaves = m_depth * maxSweepOctaves();
    m_coefficientLeft = coefficientForFrequency(m_centreFrequency * std::exp2(m_lfoLeft.nextSample() * octaves));
    m_coefficientRight = coefficientForFrequency(m_centreFrequency * std::exp2(m_lfoRight.nextSample() * octaves));
}

double Phaser::Cascade::process(double input, int stages, double coefficient, double feedback)
{
    double sample = input + feedbackSample * feedback;

    for (int stage = 0; stage < stages; stage++) {
        const auto index = static_cast<size_t>(stage);
        const double output = coefficient * sample + x1.at(index) - coefficient * y1.at(index);
        x1.at(index) = sample;
        y1.at(index) = output;
        sample = output;
    }

    // An all-pass cascade passes everything at unity, so a feedback amount short of one cannot run
    // away. A denormal or a NaN arriving from elsewhere still can, and would stay in the loop.
    if (!std::isfinite(sample)) {
        reset();
        return 0.0;
    }

    feedbackSample = sample;

    return sample;
}

void Phaser::Cascade::reset()
{
    x1.fill(0.0);
    y1.fill(0.0);
    feedbackSample = 0.0;
}

void Phaser::processSample(double & left, double & right)
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

    if (!m_controlCounter) {
        updateModulation();
    }
    if (++m_controlCounter >= controlRateStride) {
        m_controlCounter = 0;
    }

    left = m_left.process(left, m_stages, m_coefficientLeft, m_feedback) * m_gain;
    right = m_right.process(right, m_stages, m_coefficientRight, m_feedback) * m_gain;
}

void Phaser::sync()
{
    // Applied on the audio thread, which is the only thread the cascades and the LFOs are touched
    // from.
    m_shouldApplyParameters = true;
}

void Phaser::setBpm(float bpm)
{
    Effect::setBpm(bpm);
    m_shouldApplyParameters = true;
}

void Phaser::reset()
{
    m_left.reset();
    m_right.reset();
    m_lfoLeft.reset();
    m_lfoRight.reset();
    m_controlCounter = 0;
    // Resetting the LFOs put both back to phase zero, so the stereo offset has to be laid on again
    // before the first sample of the next run.
    m_shouldApplyParameters = true;
}

} // namespace noteahead
