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

#include "parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

double ParameterMapper::mapExponential(double value, double min, double max)
{
    if (min <= 0 || max <= 0) return min;
    return min * std::pow(max / min, std::clamp(value, 0.0, 1.0));
}

double ParameterMapper::unmapExponential(double mappedValue, double min, double max)
{
    if (min <= 0 || max <= 0 || mappedValue <= 0) return 0.0;
    return std::log(mappedValue / min) / std::log(max / min);
}

double ParameterMapper::mapCubic(double value, double min, double max)
{
    const double range = max - min;
    return min + (std::pow(std::clamp(value, 0.0, 1.0), 3.0) * range);
}

double ParameterMapper::unmapCubic(double mappedValue, double min, double max)
{
    const double range = max - min;
    if (range <= 0) return 0.0;
    const double norm = std::max(0.0, std::min(1.0, (mappedValue - min) / range));
    return std::pow(norm, 1.0 / 3.0);
}

double ParameterMapper::mapCubicCentered(double value, double min, double max)
{
    const double center = (min + max) / 2.0;
    const double range = (max - min) / 2.0;
    const double norm = std::clamp(value, -1.0, 1.0);
    const double mapped = (norm >= 0 ? 1.0 : -1.0) * std::pow(std::abs(norm), 3.0);
    return mapped * range + center;
}

double ParameterMapper::unmapCubicCentered(double mappedValue, double min, double max)
{
    const double center = (min + max) / 2.0;
    const double range = (max - min) / 2.0;
    if (range == 0) return 0.0;
    const double norm = std::max(-1.0, std::min(1.0, (mappedValue - center) / range));
    return (norm >= 0 ? 1.0 : -1.0) * std::pow(std::abs(norm), 1.0 / 3.0);
}

double ParameterMapper::mapLogFrequency(double value, double min, double max)
{
    // Use the formula from Utils::Dsp::cutoffToHz
    // f = maxFreq * (pow(1000.0, cutoff) - 1.0) / 999.0
    // But generalized for min/max. 
    // Actually, Utils::Dsp::cutoffToHz is specifically for [0, 1] -> [0, 20000].
    // Let's stick to a more standard exponential mapping if min > 0.
    if (min <= 0) {
        return max * (std::pow(1000.0, std::clamp(value, 0.0, 1.0)) - 1.0) / 999.0;
    }
    return mapExponential(value, min, max);
}

double ParameterMapper::unmapLogFrequency(double mappedValue, double min, double max)
{
    if (min <= 0) {
        if (mappedValue <= 0) return 0.0;
        return std::log(mappedValue * 999.0 / max + 1.0) / std::log(1000.0);
    }
    return unmapExponential(mappedValue, min, max);
}

double ParameterMapper::mapDecibel(double value, double range)
{
    const double db = (value - 0.5) * (range * 2.0);
    return std::pow(10.0, db / 20.0);
}

double ParameterMapper::unmapDecibel(double mappedValue, double range)
{
    if (mappedValue <= 0) return 0.0;
    const double db = 20.0 * std::log10(mappedValue);
    return (db / (range * 2.0)) + 0.5;
}

double ParameterMapper::mapLfoFrequency(double value, double /*min*/, double /*max*/)
{
    // Use the specific formula from SynthDevice: std::pow(20.0f, x) - 0.95f
    // This maps 0..1 to 0.05..19.05 Hz. 
    return std::pow(20.0, std::clamp(value, 0.0, 1.0)) - 0.95;
}

double ParameterMapper::unmapLfoFrequency(double mappedValue, double /*min*/, double /*max*/)
{
    return std::log(std::max(0.0001, mappedValue + 0.95)) / std::log(20.0);
}

} // namespace noteahead
