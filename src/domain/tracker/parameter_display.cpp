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

#include "parameter_display.hpp"

#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

double ParameterDisplay::toUnits(double position) const
{
    const auto clamped = std::clamp(position, 0.0, 1.0);
    switch (mapping) {
    case Mapping::Exponential:
        return ParameterMapper::mapExponential(clamped, min, max);
    case Mapping::CubicCentered:
        return ParameterMapper::mapCubicCentered(clamped * 2.0 - 1.0, min, max);
    case Mapping::LogFrequency:
    case Mapping::LogFrequencyHighPass:
        return ParameterMapper::mapLogFrequency(clamped, min, max);
    case Mapping::LfoFrequency:
        return ParameterMapper::mapLfoFrequency(clamped, min, max);
    case Mapping::ExponentialFromZero:
        return ParameterMapper::mapExponentialFromZero(clamped, min, max);
    case Mapping::Cubic:
        return ParameterMapper::mapCubic(clamped, min, max);
    case Mapping::Fader: {
        // Floored for the same reason the equalizers floor their curves: the bottom is silence
        constexpr double floorGain = 1.0e-6;
        return 20.0 * std::log10(std::max(floorGain, ParameterMapper::mapFader(clamped)));
    }
    case Mapping::Linear:
    case Mapping::Pan:
    case Mapping::Bipolar:
        break;
    }
    return min + clamped * (max - min);
}

double ParameterDisplay::toPosition(double units) const
{
    double position = 0.0;
    switch (mapping) {
    case Mapping::Exponential:
        position = ParameterMapper::unmapExponential(units, min, max);
        break;
    case Mapping::CubicCentered:
        position = (ParameterMapper::unmapCubicCentered(units, min, max) + 1.0) / 2.0;
        break;
    case Mapping::LogFrequency:
    case Mapping::LogFrequencyHighPass:
        position = ParameterMapper::unmapLogFrequency(units, min, max);
        break;
    case Mapping::LfoFrequency:
        position = ParameterMapper::unmapLfoFrequency(units, min, max);
        break;
    case Mapping::ExponentialFromZero:
        position = ParameterMapper::unmapExponentialFromZero(units, min, max);
        break;
    case Mapping::Cubic:
        position = ParameterMapper::unmapCubic(units, min, max);
        break;
    case Mapping::Fader:
        position = ParameterMapper::unmapFader(std::pow(10.0, units / 20.0));
        break;
    case Mapping::Linear:
    case Mapping::Pan:
    case Mapping::Bipolar:
        position = max != min ? (units - min) / (max - min) : 0.0;
        break;
    }
    return std::clamp(position, 0.0, 1.0);
}

double ParameterDisplay::restPosition() const
{
    return rest == Rest::Centre ? 0.5 : 0.0;
}

std::string ParameterDisplay::mappingName() const
{
    switch (mapping) {
    case Mapping::Exponential:
        return "exponential";
    case Mapping::CubicCentered:
        return "cubicCentered";
    case Mapping::LogFrequency:
        return "logFrequency";
    case Mapping::LogFrequencyHighPass:
        return "logFrequencyHighPass";
    case Mapping::LfoFrequency:
        return "lfoFrequency";
    case Mapping::ExponentialFromZero:
        return "exponentialFromZero";
    case Mapping::Pan:
        return "pan";
    case Mapping::Fader:
        return "fader";
    case Mapping::Bipolar:
        return "bipolar";
    case Mapping::Cubic:
        return "cubic";
    case Mapping::Linear:
        break;
    }
    return "linear";
}

} // namespace noteahead
