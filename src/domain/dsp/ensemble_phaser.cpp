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

#include "ensemble_phaser.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double MinRateHz { 0.05 };
constexpr double MaxRateHz { 6.0 };

//! Sweep range of the all-pass corner. The notches land where the strings have most of their
//! energy, which is what makes the effect audible on a pad rather than just on transients.
constexpr double MinSweepHz { 200.0 };
constexpr double MaxSweepHz { 2400.0 };

constexpr double MaxFeedback { 0.7 };

} // namespace

EnsemblePhaser::EnsemblePhaser()
{
    m_lfoLeft.setWaveform(Lfo::Waveform::Sine);
    m_lfoRight.setWaveform(Lfo::Waveform::Sine);
    m_lfoLeft.setPhase(0.0);
    m_lfoRight.setPhase(0.25); // Quadrature: the sweep reaches each channel a quarter cycle apart

    // setSampleRate() skips the update when the rate already matches the default, so the LFOs have
    // to be given the starting rate here rather than waiting for a call that may never come.
    m_lfoLeft.setSampleRate(m_sampleRate);
    m_lfoRight.setSampleRate(m_sampleRate);
    setRate(m_rate);
}

void EnsemblePhaser::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    m_lfoLeft.setSampleRate(sampleRate);
    m_lfoRight.setSampleRate(sampleRate);
}

void EnsemblePhaser::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool EnsemblePhaser::enabled() const
{
    return m_enabled;
}

void EnsemblePhaser::setRate(double rate)
{
    m_rate = std::clamp(rate, 0.0, 1.0);
    const double rateHz = MinRateHz * std::pow(MaxRateHz / MinRateHz, m_rate);
    m_lfoLeft.setFrequency(rateHz);
    m_lfoRight.setFrequency(rateHz);
}

double EnsemblePhaser::rate() const
{
    return m_rate;
}

void EnsemblePhaser::setColor(double color)
{
    m_color = std::clamp(color, 0.0, 1.0);
}

double EnsemblePhaser::color() const
{
    return m_color;
}

double EnsemblePhaser::coefficientForFrequency(double frequency) const
{
    if (m_sampleRate <= 0.0) {
        return 0.0;
    }
    const double maxFrequency = m_sampleRate * 0.49;
    const double tangent = std::tan(std::numbers::pi * std::min(frequency, maxFrequency) / m_sampleRate);
    return (tangent - 1.0) / (tangent + 1.0);
}

double EnsemblePhaser::Channel::process(double input, double coefficient, double feedback)
{
    double sample = input + feedbackSample * feedback;

    for (int stage = 0; stage < StageCount; stage++) {
        const auto index = static_cast<size_t>(stage);
        const double output = coefficient * sample + x1.at(index) - coefficient * y1.at(index);
        x1.at(index) = sample;
        y1.at(index) = output;
        sample = output;
    }

    feedbackSample = sample;

    return sample;
}

void EnsemblePhaser::Channel::reset()
{
    x1.fill(0.0);
    y1.fill(0.0);
    feedbackSample = 0.0;
}

void EnsemblePhaser::process(double & left, double & right)
{
    if (m_sampleRate <= 0.0) {
        return;
    }

    // The sweep is centred and its span opens up with Color, so the notches both deepen and travel
    // further as the knob is turned.
    const double depth = 0.3 + m_color * 0.7;
    const double centre = std::sqrt(MinSweepHz * MaxSweepHz);
    const double span = std::log(MaxSweepHz / MinSweepHz) * 0.5 * depth;

    const double sweepLeft = centre * std::exp(m_lfoLeft.nextSample() * span);
    const double sweepRight = centre * std::exp(m_lfoRight.nextSample() * span);

    const double feedback = m_color * MaxFeedback;

    const double wetLeft = m_left.process(left, coefficientForFrequency(sweepLeft), feedback);
    const double wetRight = m_right.process(right, coefficientForFrequency(sweepRight), feedback);

    if (m_enabled) {
        left = (left + wetLeft) * 0.5;
        right = (right + wetRight) * 0.5;
    }
}

void EnsemblePhaser::reset()
{
    m_left.reset();
    m_right.reset();
    m_lfoLeft.reset();
    m_lfoRight.reset();
    m_lfoLeft.setPhase(0.0);
    m_lfoRight.setPhase(0.25);
}

} // namespace noteahead
