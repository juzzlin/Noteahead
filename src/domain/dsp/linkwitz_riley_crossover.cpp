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

#include "linkwitz_riley_crossover.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Butterworth damping, 1 / Q with Q = 1 / sqrt(2). Cascading two sections at this damping is what
//! makes the pair Linkwitz-Riley rather than merely fourth order.
const double butterworthDamping = std::numbers::sqrt2;

//! Keep the corner clear of Nyquist, where the bilinear prewarp blows up and then changes sign.
constexpr double maxNyquistRatio = 0.49;

constexpr double minCutoff = 20.0;

} // namespace

void LinkwitzRileyCrossover::setCutoff(double frequency)
{
    m_cutoff = frequency;
}

double LinkwitzRileyCrossover::cutoff() const
{
    return m_cutoff;
}

void LinkwitzRileyCrossover::updateCoefficients()
{
    if (std::abs(m_cutoff - m_lastCutoff) < 1.0e-6 && std::abs(m_sampleRate - m_lastSampleRate) < 0.1) {
        return;
    }

    const double limit = m_sampleRate * maxNyquistRatio;
    const double frequency = std::clamp(m_cutoff, minCutoff, std::max(minCutoff, limit));

    m_g = std::tan(std::numbers::pi * frequency / m_sampleRate);
    m_k = butterworthDamping;
    m_den = 1.0 / (1.0 + m_g * (m_g + m_k));

    m_lastCutoff = m_cutoff;
    m_lastSampleRate = m_sampleRate;
}

double LinkwitzRileyCrossover::processLowPass(Section & section, double input) const
{
    const double hp = (input - (m_g + m_k) * section.s1 - section.s2) * m_den;
    const double v1 = m_g * hp;
    const double bp = v1 + section.s1;
    section.s1 = v1 + bp;
    const double v2 = m_g * bp;
    const double lp = v2 + section.s2;
    section.s2 = v2 + lp;

    return lp;
}

double LinkwitzRileyCrossover::processHighPass(Section & section, double input) const
{
    const double hp = (input - (m_g + m_k) * section.s1 - section.s2) * m_den;
    const double v1 = m_g * hp;
    const double bp = v1 + section.s1;
    section.s1 = v1 + bp;
    const double v2 = m_g * bp;
    const double lp = v2 + section.s2;
    section.s2 = v2 + lp;

    return hp;
}

void LinkwitzRileyCrossover::process(double input, double & low, double & high)
{
    if (m_sampleRate <= 0.0) {
        low = input;
        high = 0.0;
        return;
    }

    updateCoefficients();

    low = processLowPass(m_lowSecond, processLowPass(m_lowFirst, input));
    high = processHighPass(m_highSecond, processHighPass(m_highFirst, input));

    if (std::isnan(low) || std::isnan(high)) {
        reset();
        low = 0.0;
        high = 0.0;
    }
}

double LinkwitzRileyCrossover::processAllPass(double input)
{
    double low = 0.0;
    double high = 0.0;
    process(input, low, high);

    return low + high;
}

void LinkwitzRileyCrossover::reset()
{
    m_lowFirst = {};
    m_lowSecond = {};
    m_highFirst = {};
    m_highSecond = {};
}

} // namespace noteahead
