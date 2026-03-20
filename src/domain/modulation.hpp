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

#ifndef MODULATION_HPP
#define MODULATION_HPP

#include <cstddef>

namespace noteahead {

struct ModulationParameters
{
    enum class ModulationType
    {
        SineWave,
        Random
    };

    ModulationType type = ModulationType::SineWave;
    float cycles = 0.f;
    float amplitude = 0.f;
    float offset = 0.f;
    bool inverted = false;

    bool operator==(const ModulationParameters & other) const
    {
        return type == other.type && cycles == other.cycles && amplitude == other.amplitude && offset == other.offset && inverted == other.inverted;
    }

    bool operator!=(const ModulationParameters & other) const
    {
        return !(*this == other);
    }
};

} // namespace noteahead

#endif // MODULATION_HPP
