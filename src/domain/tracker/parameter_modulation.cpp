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

#include "parameter_modulation.hpp"

#include "../../common/parameter_mapper.hpp"
#include "parameter.hpp"

#include <algorithm>

namespace noteahead::ParameterModulation {

float modulatedValue(Mode mode, double source, double depth, const Parameter & parameter)
{
    const double base = static_cast<double>(parameter.unmodulatedValue());
    const double clampedSource = std::clamp(source, -1.0, 1.0);
    const double clampedDepth = std::clamp(depth, 0.0, 1.0);
    // Unipolar and Tremolo go one way only: the bottom of the source is the setting itself
    const double amount = clampedDepth * (clampedSource + 1.0) * 0.5;
    const auto & display = parameter.display();

    switch (mode) {
    case Mode::Bipolar:
        return static_cast<float>(std::clamp(base + clampedSource * clampedDepth * 0.5, 0.0, 1.0));
    case Mode::Tremolo:
        if (display && display->mapping == ParameterDisplay::Mapping::Fader) {
            return static_cast<float>(ParameterMapper::unmapFader(ParameterMapper::mapFader(base) * (1.0 - amount)));
        }
        [[fallthrough]];
    case Mode::Unipolar:
        break;
    }
    const double rest = display ? display->restPosition() : 0.0;
    return static_cast<float>(rest + (base - rest) * (1.0 - amount));
}

} // namespace noteahead::ParameterModulation
