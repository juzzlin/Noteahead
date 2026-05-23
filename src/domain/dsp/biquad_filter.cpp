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

#include "biquad_filter.hpp"

#include <cmath>
#include <numbers>

namespace noteahead {

void BiquadFilter::calculateBell(double frequency, double sampleRate, double q, double gainDb)
{
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double A = std::pow(10.0, gainDb / 40.0);
    const double cosW0 = std::cos(w0);

    const double b0 = 1.0 + alpha * A;
    const double b1 = -2.0 * cosW0;
    const double b2 = 1.0 - alpha * A;
    const double a0 = 1.0 + alpha / A;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha / A;

    m_coefficients = { a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0 };
    m_isBypassed = false;
}

void BiquadFilter::calculateLowShelf(double frequency, double sampleRate, double q, double gainDb)
{
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double A = std::pow(10.0, gainDb / 40.0);
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosW0 = std::cos(w0);
    const double sqrtA2 = 2.0 * std::sqrt(A) * alpha;

    const double b0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + sqrtA2);
    const double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
    const double b2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - sqrtA2);
    const double a0 = (A + 1.0) + (A - 1.0) * cosW0 + sqrtA2;
    const double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
    const double a2 = (A + 1.0) + (A - 1.0) * cosW0 - sqrtA2;

    m_coefficients = { a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0 };
    m_isBypassed = false;
}

void BiquadFilter::calculateHighShelf(double frequency, double sampleRate, double q, double gainDb)
{
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double A = std::pow(10.0, gainDb / 40.0);
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosW0 = std::cos(w0);
    const double sqrtA2 = 2.0 * std::sqrt(A) * alpha;

    const double b0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + sqrtA2);
    const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
    const double b2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - sqrtA2);
    const double a0 = (A + 1.0) - (A - 1.0) * cosW0 + sqrtA2;
    const double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
    const double a2 = (A + 1.0) - (A - 1.0) * cosW0 - sqrtA2;

    m_coefficients = { a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0 };
    m_isBypassed = false;
}

void BiquadFilter::calculateLowCut(double frequency, double sampleRate, double q)
{
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosW0 = std::cos(w0);

    const double b0 = (1.0 + cosW0) / 2.0;
    const double b1 = -(1.0 + cosW0);
    const double b2 = (1.0 + cosW0) / 2.0;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    m_coefficients = { a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0 };
    m_isBypassed = false;
}

void BiquadFilter::calculateHighCut(double frequency, double sampleRate, double q)
{
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosW0 = std::cos(w0);

    const double b0 = (1.0 - cosW0) / 2.0;
    const double b1 = 1.0 - cosW0;
    const double b2 = (1.0 - cosW0) / 2.0;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    m_coefficients = { a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0 };
    m_isBypassed = false;
}

void BiquadFilter::calculateNotch(double frequency, double sampleRate, double q)
{
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double alpha = std::sin(w0) / (2.0 * q);
    const double cosW0 = std::cos(w0);

    const double b0 = 1.0;
    const double b1 = -2.0 * cosW0;
    const double b2 = 1.0;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    m_coefficients = { a1 / a0, a2 / a0, b0 / a0, b1 / a0, b2 / a0 };
    m_isBypassed = false;
}

void BiquadFilter::setBypass()
{
    m_isBypassed = true;
}

float BiquadFilter::process(float input)
{
    if (m_isBypassed) {
        return input;
    }

    const double out = m_coefficients.b0 * input + m_z1;
    m_z1 = m_coefficients.b1 * input - m_coefficients.a1 * out + m_z2;
    m_z2 = m_coefficients.b2 * input - m_coefficients.a2 * out;

    // Denormal protection
    if (std::abs(m_z1) < 1.0e-15)
        m_z1 = 0.0;
    if (std::abs(m_z2) < 1.0e-15)
        m_z2 = 0.0;

    return static_cast<float>(out);
}

void BiquadFilter::reset()
{
    m_z1 = 0.0;
    m_z2 = 0.0;
}

} // namespace noteahead
