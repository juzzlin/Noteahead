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

#include "saturating_svf.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Corner limits. The upper one is a fraction of Nyquist: tan() runs away at Nyquist itself.
constexpr double MinCutoffHz = 20.0;
constexpr double NyquistMargin = 0.45;

//! Damping at either end of the resonance control. 2 is critically damped, and the top stops short
//! of zero so the linear part of the loop always keeps a little damping of its own.
constexpr double MinDamping = 0.05;
constexpr double MaxDamping = 2.0;

//! Below this the saturator is left out entirely rather than dividing by a vanishing number.
constexpr double MinSaturation = 1.0e-4;

} // namespace

void SaturatingSvf::setCutoff(double frequency)
{
    m_cutoff = frequency;
}

void SaturatingSvf::setResonance(double resonance)
{
    m_resonance = std::clamp(resonance, 0.0, 1.0);
}

void SaturatingSvf::setSaturation(double drive)
{
    m_saturation = std::max(0.0, drive);
}

void SaturatingSvf::setSaturationPerStep(double fraction)
{
    m_saturationPerStep = std::clamp(fraction, 0.0, 1.0);
}

void SaturatingSvf::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
}

void SaturatingSvf::updateCoefficients()
{
    if (m_cutoff == m_lastCutoff && m_resonance == m_lastResonance && m_sampleRate == m_lastSampleRate) {
        return;
    }

    const double sampleRate = m_sampleRate > 0.0 ? m_sampleRate : 48000.0;
    const double cutoff = std::clamp(m_cutoff, MinCutoffHz, sampleRate * NyquistMargin);

    m_g = std::tan(std::numbers::pi * cutoff / sampleRate);
    m_k = MaxDamping - (MaxDamping - MinDamping) * m_resonance;
    m_a1 = 1.0 / (1.0 + m_g * (m_g + m_k));
    m_a2 = m_g * m_a1;
    m_a3 = m_g * m_a2;

    m_lastCutoff = m_cutoff;
    m_lastResonance = m_resonance;
    m_lastSampleRate = m_sampleRate;
}

double SaturatingSvf::process(double input)
{
    updateCoefficients();

    // Ordinary TPT solve: the resonance feedback is still resolved without delay, so the corner and
    // the peak land where the coefficients say they do.
    const double v3 = input - m_ic2eq;
    const double v1 = m_a1 * m_ic1eq + m_a2 * v3;
    const double v2 = m_ic2eq + m_a2 * m_ic1eq + m_a3 * v3;

    m_bandPass = v1;
    m_lowPass = v2;
    m_highPass = input - m_k * v1 - v2;

    double s1 = 2.0 * v1 - m_ic1eq;
    double s2 = 2.0 * v2 - m_ic2eq;

    // The integrators are where the analog circuit runs out of headroom, so that is where the
    // ceiling goes. Everything the loop does under drive -- the peak folding down, the harmonics
    // coming back round through the poles -- follows from these two lines.
    if (m_saturation > MinSaturation) {
        const double scale = 1.0 / m_saturation;
        // Only a share of the way towards the squashed value on each step, so that running four
        // times as often does not squash four times as hard. See setSaturationPerStep().
        s1 += (std::tanh(s1 * m_saturation) * scale - s1) * m_saturationPerStep;
        s2 += (std::tanh(s2 * m_saturation) * scale - s2) * m_saturationPerStep;
    }

    m_ic1eq = s1;
    m_ic2eq = s2;

    return m_lowPass;
}

double SaturatingSvf::lowPass() const
{
    return m_lowPass;
}

double SaturatingSvf::bandPass() const
{
    return m_bandPass;
}

double SaturatingSvf::highPass() const
{
    return m_highPass;
}

void SaturatingSvf::reset()
{
    m_ic1eq = 0.0;
    m_ic2eq = 0.0;
    m_lowPass = 0.0;
    m_bandPass = 0.0;
    m_highPass = 0.0;
}

} // namespace noteahead
