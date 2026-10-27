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

#include "analog_drift.hpp"

#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! Slowest and fastest the wander's main sine runs, in Hz: slow enough to read as tuning that will
//! not sit still rather than as vibrato.
constexpr double MinRateHz { 0.05 };
constexpr double RateSpreadHz { 0.28 };
//! Fractional parts of the golden and silver ratios.
constexpr double GoldenRatioFraction { 0.6180339887498949 };
constexpr double SilverRatioFraction { 0.4142135623730951 };
//! The other two sines' rates against the main one's, 1/sqrt(2) and the golden ratio, and the
//! weights of all three. The weights sum to one, so the wander still stays within -1 to 1, and the
//! main sine dominates, so the index's rate still decides how fast it moves.
constexpr std::array<double, 3> PartialRatios { 1.0, 0.7071067811865476, 1.6180339887498949 };
constexpr std::array<double, 3> PartialWeights { 0.5, 0.3, 0.2 };

} // namespace

void AnalogDrift::initialize(size_t index)
{
    const auto position = static_cast<double>(index + 1);
    m_rateHz = MinRateHz + RateSpreadHz * std::fmod(position * GoldenRatioFraction, 1.0);
    for (size_t i = 0; i < PartialCount; i++) {
        m_phases[i] = std::fmod(position * SilverRatioFraction * static_cast<double>(i + 1), 1.0);
    }
}

double AnalogDrift::next(double sampleRate)
{
    double value = 0.0;
    for (size_t i = 0; i < PartialCount; i++) {
        m_phases[i] = std::fmod(m_phases[i] + m_rateHz * PartialRatios[i] / sampleRate, 1.0);
        value += PartialWeights[i] * std::sin(m_phases[i] * 2.0 * std::numbers::pi);
    }
    return value;
}

double AnalogDrift::tolerance(size_t index, size_t component)
{
    // A different multiple of the golden ratio per component spreads each one evenly over the
    // voices, in an order of its own.
    const auto position = static_cast<double>(index + 1);
    const auto multiple = static_cast<double>(component + 1);
    return 2.0 * std::fmod(position * multiple * GoldenRatioFraction + static_cast<double>(component) * SilverRatioFraction, 1.0) - 1.0;
}

} // namespace noteahead
