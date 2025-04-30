// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef AUTOMATION_LOCATION_HPP
#define AUTOMATION_LOCATION_HPP

#include <cstddef>

namespace noteahead {

struct AutomationLocation
{
    size_t pattern = 0;
    size_t track = 0;
    size_t column = 0;

    bool operator==(const AutomationLocation & other) const
    {
        return pattern == other.pattern && track == other.track && column == other.column;
    }

    bool operator!=(const AutomationLocation & other) const
    {
        return !(*this == other);
    }
};

} // namespace noteahead

#endif // AUTOMATION_LOCATION_HPP
