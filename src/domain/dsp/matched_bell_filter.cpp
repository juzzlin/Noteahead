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

#include "matched_bell_filter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! The analog bell's squared magnitude at @p ratio times its centre frequency.
double analogBellMagnitudeSquared(double ratio, double k, double gain)
{
    const double real = 1.0 - ratio * ratio;
    const double numeratorImaginary = k * gain * gain * ratio;
    const double denominatorImaginary = k * ratio;
    return (real * real + numeratorImaginary * numeratorImaginary) / (real * real + denominatorImaginary * denominatorImaginary);
}

} // namespace

void MatchedBellFilter::calculate(double frequency, double sampleRate, double q, double gainDb)
{
    if (sampleRate <= 0.0 || frequency <= 0.0 || frequency >= sampleRate * 0.5 || q <= 0.0 || gainDb == 0.0) {
        setBypass();
        return;
    }

    calculateBoost(frequency, sampleRate, q, std::abs(gainDb));

    if (gainDb < 0.0) {
        // The inverse of the boost: numerator and denominator trade places, renormalised so that the
        // denominator leads with 1 again.
        const double b0 = m_b0;
        const double b1 = m_b1;
        const double b2 = m_b2;
        m_b0 = 1.0 / b0;
        m_b1 = m_a1 / b0;
        m_b2 = m_a2 / b0;
        m_a1 = b1 / b0;
        m_a2 = b2 / b0;
    }

    m_isBypassed = false;
}

void MatchedBellFilter::calculateBoost(double frequency, double sampleRate, double q, double gainDb)
{
    const double gain = std::pow(10.0, gainDb / 40.0);
    const double k = 1.0 / (q * gain);
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;

    // Impulse-invariant poles. A bell of any useful Q is under-damped, but the over-damped branch
    // keeps a very wide one from taking the square root of a negative number.
    const double zeta = k * 0.5;
    const double decay = std::exp(-zeta * w0);
    m_a1 = zeta <= 1.0 ? -2.0 * decay * std::cos(w0 * std::sqrt(1.0 - zeta * zeta)) : -2.0 * decay * std::cosh(w0 * std::sqrt(zeta * zeta - 1.0));
    m_a2 = decay * decay;

    // The squared magnitude of a biquad is linear in these three, at phi0 = cos^2(w/2),
    // phi1 = sin^2(w/2) and phi2 = 4 phi0 phi1. The denominator's are fixed by the poles; the
    // numerator's are solved so that the ratio equals the analog response at DC (phi1 = 0), at
    // Nyquist (phi1 = 1) and at the centre.
    const double a0Squared = (1.0 + m_a1 + m_a2) * (1.0 + m_a1 + m_a2);
    const double a1Squared = (1.0 - m_a1 + m_a2) * (1.0 - m_a1 + m_a2);
    const double a2Term = -4.0 * m_a2;
    const double phi1 = std::sin(w0 * 0.5) * std::sin(w0 * 0.5);
    const double phi0 = 1.0 - phi1;
    const double phi2 = 4.0 * phi0 * phi1;

    const double numerator0 = a0Squared * analogBellMagnitudeSquared(0.0, k, gain);
    const double numerator1 = a1Squared * analogBellMagnitudeSquared(std::numbers::pi / w0, k, gain);
    const double centre = analogBellMagnitudeSquared(1.0, k, gain);
    const double numerator2 = (centre * (a0Squared * phi0 + a1Squared * phi1 + a2Term * phi2) - numerator0 * phi0 - numerator1 * phi1) / phi2;

    // Back from the squared terms to the coefficients, choosing the root that keeps the zeros
    // inside the unit circle so that the inverse used for a cut is stable. The discriminant is
    // positive for every bell a graphic equalizer asks for; the clamp only guards rounding.
    const double sum0 = std::sqrt(numerator0);
    const double sum1 = std::sqrt(numerator1);
    const double outer = (sum0 + sum1) * 0.5;
    const double root = std::sqrt(std::max(0.0, outer * outer + numerator2));
    m_b0 = (outer + root) * 0.5;
    m_b1 = (sum0 - sum1) * 0.5;
    m_b2 = -numerator2 / (4.0 * m_b0);
}

void MatchedBellFilter::setBypass()
{
    m_b0 = 1.0;
    m_b1 = 0.0;
    m_b2 = 0.0;
    m_a1 = 0.0;
    m_a2 = 0.0;
    m_isBypassed = true;
}

bool MatchedBellFilter::isBypassed() const
{
    return m_isBypassed;
}

std::complex<double> MatchedBellFilter::responseAt(double frequency, double sampleRate) const
{
    if (m_isBypassed || sampleRate <= 0.0) {
        return { 1.0, 0.0 };
    }
    const double w = 2.0 * std::numbers::pi * std::clamp(frequency, 0.0, sampleRate * 0.5) / sampleRate;
    const std::complex<double> z1 = std::polar(1.0, -w);
    const std::complex<double> z2 = z1 * z1;
    return (m_b0 + m_b1 * z1 + m_b2 * z2) / (1.0 + m_a1 * z1 + m_a2 * z2);
}

double MatchedBellFilter::magnitudeDbAt(double frequency, double sampleRate) const
{
    return 20.0 * std::log10(std::abs(responseAt(frequency, sampleRate)));
}

MatchedBellFilter::SquaredMagnitude MatchedBellFilter::squaredMagnitude() const
{
    if (m_isBypassed) {
        return {};
    }
    const auto terms = [](double c0, double c1, double c2) {
        return std::array<double, 3> { (c0 + c1 + c2) * (c0 + c1 + c2), (c0 - c1 + c2) * (c0 - c1 + c2), -4.0 * c0 * c2 };
    };
    return { terms(m_b0, m_b1, m_b2), terms(1.0, m_a1, m_a2) };
}

double MatchedBellFilter::process(double input)
{
    if (m_isBypassed) {
        return input;
    }
    // Transposed direct form II: the form whose state stays well scaled for poles this close to
    // the unit circle, which a 16 Hz band at a high sample rate puts them.
    const double output = m_b0 * input + m_z1;
    m_z1 = m_b1 * input - m_a1 * output + m_z2;
    m_z2 = m_b2 * input - m_a2 * output;
    return output;
}

void MatchedBellFilter::reset()
{
    m_z1 = 0.0;
    m_z2 = 0.0;
}

} // namespace noteahead
