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

#include "flanger.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! What the Delay control spans, in milliseconds: the shortest delay the sweep reaches.
//!
//! Above about ten milliseconds the copy stops combing and starts being heard as a separate sound,
//! which is a chorus rather than a flanger. The floor is a fraction of a millisecond because that
//! is where the first notch is above the audible band and the comb is heard as a thinning rather
//! than as a pitch.
constexpr double MinDelayMs = 0.2;
constexpr double MaxDelayMs = 10.0;

//! How far the sweep travels above the Delay setting at full depth, as a multiple of it. The sweep
//! is multiplicative rather than additive because the comb's notches are harmonically spaced: a
//! fixed number of milliseconds moves them a great deal at a short delay and hardly at all at a
//! long one, which makes Depth mean something different at each end of Delay.
constexpr double DepthRange = 8.0;

//! Largest feedback either way. Short of one, because the comb's peaks are already resonant and a
//! loop at unity around a delay this short rings on a single frequency rather than colouring.
constexpr double MaxFeedback = 0.92;

//! Headroom for the sweep, so the delay line is long enough for the longest delay the controls can
//! ask for however the sample rate moves under it.
constexpr double MaxDelaySeconds = MaxDelayMs * (1.0 + DepthRange) * 0.001;

} // namespace

Flanger::Flanger()
{
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoRate().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyLfoMode().toStdString(), 0.0f, 0, 2, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyRateDivider().toStdString(), 1.0f, 1, maxRateDivider(), 1, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDepth().toStdString(), 0.6f, 0, 10000, 6000, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyDelay().toStdString(), 0.1f, 0, 10000, 1000, 100 });
    // Centred, so half travel is no feedback and either side is one of the two polarities.
    addParameter(Parameter { Constants::NahdXml::xmlKeyFeedback().toStdString(), 0.5f, -10000, 10000, 0, 100 });
    addParameter(Parameter { Constants::NahdXml::xmlKeyStereoPhase().toStdString(), 0.5f, 0, 180, 90 });
    addMixParameter(0.5f, MixLaw::DualSlope);
}

int Flanger::maxRateDivider()
{
    return 64;
}

std::string Flanger::typeIdString()
{
    return "a7b8c9d0-e1f2-4a3b-5c6d-7e8f9a0b1c2d";
}

std::string Flanger::type() const
{
    return Constants::RackEffectType::flanger().toStdString();
}

std::string Flanger::typeId() const
{
    return typeIdString();
}

void Flanger::processSample(double & left, double & right)
{
    // Zero to one rather than the LFO's own bipolar swing: the sweep runs upwards from the Delay
    // setting, so that turning Depth down lands on that setting rather than somewhere in the
    // middle of a range the user cannot see.
    const double sweepLeft = (m_lfoLeft.nextSample() + 1.0) * 0.5;
    const double sweepRight = (m_lfoRight.nextSample() + 1.0) * 0.5;

    const double baseSamples = m_delayMs * 0.001 * m_sampleRate;
    m_delayLeft.setFractionalDelay(baseSamples * (1.0 + DepthRange * m_depth * sweepLeft));
    m_delayRight.setFractionalDelay(baseSamples * (1.0 + DepthRange * m_depth * sweepRight));

    const double wetLeft = m_delayLeft.read();
    const double wetRight = m_delayRight.read();

    m_delayLeft.write(left + wetLeft * m_feedback);
    m_delayRight.write(right + wetRight * m_feedback);

    // Wet only. Effect::process() holds the dry signal and blends it back, and the sum of the two
    // is what combs -- so the Mix control is not a convenience here, it is the depth of the notches.
    left = wetLeft;
    right = wetRight;
}

void Flanger::processBlock(AudioContext & context)
{
    if (m_sampleRate != context.sampleRate) {
        m_sampleRate = context.sampleRate;
        updateSampleRateDependents();
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void Flanger::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString()); p) {
        m_rate = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString()); p) {
        m_lfoMode = static_cast<Lfo::Mode>(std::clamp(p->get().xmlValue(), 0, 2));
        m_lfoLeft.setMode(m_lfoMode);
        m_lfoRight.setMode(m_lfoMode);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString()); p) {
        m_rateDivider = std::clamp(p->get().xmlValue(), 1, maxRateDivider());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDepth().toStdString()); p) {
        m_depth = static_cast<double>(p->get().value());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyDelay().toStdString()); p) {
        m_delayMs = ParameterMapper::mapExponential(p->get().value(), MinDelayMs, MaxDelayMs);
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyFeedback().toStdString()); p) {
        // The stored value is centred on a half, so half travel is no feedback at all.
        m_feedback = (static_cast<double>(p->get().value()) * 2.0 - 1.0) * MaxFeedback;
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeyStereoPhase().toStdString()); p) {
        m_stereoPhase = static_cast<double>(p->get().xmlValue());
    }

    m_lfoLeft.setWaveform(Lfo::Waveform::Triangle);
    m_lfoRight.setWaveform(Lfo::Waveform::Triangle);
    m_lfoRight.setPhase(m_stereoPhase / 360.0);
    updateLfoFrequency();
    updateSampleRateDependents();
}

void Flanger::setBpm(float bpm)
{
    Effect::setBpm(bpm);
    updateLfoFrequency();
}

void Flanger::reset()
{
    Effect::reset();
    m_delayLeft.reset();
    m_delayRight.reset();
    m_lfoLeft.reset();
    m_lfoRight.reset();
    m_feedbackLeft = 0.0;
    m_feedbackRight = 0.0;
}

void Flanger::updateLfoFrequency()
{
    // The divider sits after both, as on the Phaser and the Auto Panner: a flanger crawling over
    // several bars is a different effect from one sweeping every second.
    const double frequency = (m_lfoMode == Lfo::Mode::BPM
                                ? (static_cast<double>(bpm()) / 60.0) * (0.25 / std::max(0.0001, m_rate))
                                : ParameterMapper::mapLfoFrequency(m_rate, 0.05, 20.0))
      / m_rateDivider;
    m_lfoLeft.setFrequency(frequency);
    m_lfoRight.setFrequency(frequency);
}

void Flanger::updateSampleRateDependents()
{
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    m_lfoLeft.setSampleRate(rate);
    m_lfoRight.setSampleRate(rate);

    const auto maxSamples = static_cast<size_t>(MaxDelaySeconds * rate) + 4;
    m_delayLeft.setMaxDelay(maxSamples);
    m_delayRight.setMaxDelay(maxSamples);
}

} // namespace noteahead
