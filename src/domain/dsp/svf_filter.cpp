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

#include "svf_filter.hpp"

#include <cmath>
#include <numbers>

namespace noteahead {

void SvfFilter::calculateBell(double frequency, double sampleRate, double q, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double g = std::tan(std::numbers::pi * frequency / sampleRate);
    const double k = 1.0 / (q * A);
    const double den = 1.0 / (1.0 + g * (g + k));

    m_g = g;
    m_k = k;
    m_a1 = den;
    m_a2 = g * den;
    m_a3 = g * m_a2;
    m_m0 = 1.0;
    m_m1 = k * (A * A - 1.0);
    m_m2 = 0.0;

    m_isBypassed = false;
}

void SvfFilter::calculateLowShelf(double frequency, double sampleRate, double q, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double g = std::tan(std::numbers::pi * frequency / sampleRate) / std::sqrt(A);
    const double k = 1.0 / q;
    const double den = 1.0 / (1.0 + g * (g + k));

    m_g = g;
    m_k = k;
    m_a1 = den;
    m_a2 = g * den;
    m_a3 = g * m_a2;
    m_m0 = 1.0;
    m_m1 = k * (A - 1.0);
    m_m2 = A * A - 1.0;

    m_isBypassed = false;
}

void SvfFilter::calculateHighShelf(double frequency, double sampleRate, double q, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double g = std::tan(std::numbers::pi * frequency / sampleRate) * std::sqrt(A);
    const double k = 1.0 / q;
    const double den = 1.0 / (1.0 + g * (g + k));

    m_g = g;
    m_k = k;
    m_a1 = den;
    m_a2 = g * den;
    m_a3 = g * m_a2;
    m_m0 = A * A;
    m_m1 = k * (1.0 - A) * A;
    m_m2 = 1.0 - A * A;

    m_isBypassed = false;
}

void SvfFilter::calculateLowCut(double frequency, double sampleRate, double q)
{
    const double g = std::tan(std::numbers::pi * frequency / sampleRate);
    const double k = 1.0 / q;
    const double den = 1.0 / (1.0 + g * (g + k));

    m_g = g;
    m_k = k;
    m_a1 = den;
    m_a2 = g * den;
    m_a3 = g * m_a2;
    m_m0 = 1.0;
    m_m1 = -k;
    m_m2 = -1.0;

    m_isBypassed = false;
}

void SvfFilter::calculateHighCut(double frequency, double sampleRate, double q)
{
    const double g = std::tan(std::numbers::pi * frequency / sampleRate);
    const double k = 1.0 / q;
    const double den = 1.0 / (1.0 + g * (g + k));

    m_g = g;
    m_k = k;
    m_a1 = den;
    m_a2 = g * den;
    m_a3 = g * m_a2;
    m_m0 = 0.0;
    m_m1 = 0.0;
    m_m2 = 1.0;

    m_isBypassed = false;
}

void SvfFilter::calculateNotch(double frequency, double sampleRate, double q)
{
    const double g = std::tan(std::numbers::pi * frequency / sampleRate);
    const double k = 1.0 / q;
    const double den = 1.0 / (1.0 + g * (g + k));

    m_g = g;
    m_k = k;
    m_a1 = den;
    m_a2 = g * den;
    m_a3 = g * m_a2;
    m_m0 = 1.0;
    m_m1 = -k;
    m_m2 = 0.0;

    m_isBypassed = false;
}

void SvfFilter::setBypass()
{
    m_isBypassed = true;
}

float SvfFilter::process(float input)
{
    if (m_isBypassed) {
        return input;
    }

    const double v0 = static_cast<double>(input);
    const double v3 = v0 - m_s2;
    const double v1 = m_a1 * m_s1 + m_a2 * v3;
    const double v2 = m_s2 + m_g * v1;

    m_s1 = 2.0 * v1 - m_s1;
    m_s2 = 2.0 * v2 - m_s2;

    const double out = m_m0 * v0 + m_m1 * v1 + m_m2 * v2;

    // Denormal protection
    if (std::abs(m_s1) < 1.0e-15)
        m_s1 = 0.0;
    if (std::abs(m_s2) < 1.0e-15)
        m_s2 = 0.0;

    return static_cast<float>(out);
}

void SvfFilter::reset()
{
    m_s1 = 0.0;
    m_s2 = 0.0;
}

} // namespace noteahead
