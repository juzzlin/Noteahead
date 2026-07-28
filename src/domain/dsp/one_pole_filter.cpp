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

#include "one_pole_filter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Matches the clamp SvfFilter applies, for the same reason: the prewarp diverges at Nyquist.
constexpr double MaxNyquistRatio = 0.49;

} // namespace

double OnePoleFilter::maxCorner(double sampleRate)
{
    return sampleRate * MaxNyquistRatio;
}

void OnePoleFilter::calculate(double frequency, double sampleRate)
{
    if (sampleRate <= 0.0) {
        return;
    }

    const double g = std::tan(std::numbers::pi * std::clamp(frequency, 0.0, maxCorner(sampleRate)) / sampleRate);
    m_g = g / (1.0 + g);
}

void OnePoleFilter::process(double input)
{
    const double v = (input - m_s) * m_g;
    m_lowPass = v + m_s;
    m_s = m_lowPass + v;

    // Denormal protection
    if (std::abs(m_s) < 1.0e-15) {
        m_s = 0.0;
    }

    m_highPass = input - m_lowPass;
}

double OnePoleFilter::lowPass() const
{
    return m_lowPass;
}

double OnePoleFilter::highPass() const
{
    return m_highPass;
}

void OnePoleFilter::reset()
{
    m_s = 0.0;
    m_lowPass = 0.0;
    m_highPass = 0.0;
}

} // namespace noteahead
