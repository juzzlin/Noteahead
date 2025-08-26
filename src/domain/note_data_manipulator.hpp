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

#ifndef NOTE_DATA_MANIPULATOR_HPP
#define NOTE_DATA_MANIPULATOR_HPP

#include "../application/position.hpp"

#include <vector>

namespace noteahead {

class Song;

namespace NoteDataManipulator {

using SongW = std::weak_ptr<Song>;
using ChangedPositions = std::vector<Position>;
ChangedPositions interpolateVelocityOnColumn(SongW song, const Position & start, const Position & end, uint8_t startValue, uint8_t endValue);
ChangedPositions interpolateVelocityOnTrack(SongW song, const Position & start, const Position & end, uint8_t startValue, uint8_t endValue);

} // namespace NoteDataManipulator
} // namespace noteahead

#endif // NOTE_DATA_MANIPULATOR_HPP
