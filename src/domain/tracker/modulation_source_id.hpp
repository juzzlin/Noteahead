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

#ifndef MODULATION_SOURCE_ID_HPP
#define MODULATION_SOURCE_ID_HPP

#include <cstddef>

namespace noteahead {

//! Which modulation source wrote an offset: one LFO of an LFO device, one axis of an XY pad. A
//! parameter keeps one offset per source and runs on their sum, so two sources on the same
//! parameter add up rather than take turns.
//!
//! Never saved: the owner is the address of whatever runs the source, valid only while it does.
struct ModulationSourceId
{
    const void * owner {};
    size_t slot {};

    bool operator==(const ModulationSourceId & other) const = default;
};

} // namespace noteahead

#endif // MODULATION_SOURCE_ID_HPP
