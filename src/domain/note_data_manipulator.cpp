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

#include "note_data_manipulator.hpp"

#include "../application/position.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "interpolator.hpp"
#include "note_data.hpp"
#include "song.hpp"

namespace noteahead {

static const auto TAG = "NoteDataManipulator";

NoteDataManipulator::ChangedPositions NoteDataManipulator::interpolateVelocity(SongW song, const Position & start, const Position & end, uint8_t startValue, uint8_t endValue)
{
    juzzlin::L(TAG).info() << "Requesting linear velocity interpolation: " << start.toString() << " -> " << end.toString() << ", " << static_cast<int>(startValue) << " -> " << static_cast<int>(endValue);
    NoteDataManipulator::ChangedPositions changedPositions;
    if (const auto locked = song.lock(); locked) {
        const Interpolator interpolator { start.line, end.line, static_cast<double>(startValue), static_cast<double>(endValue) };
        for (auto line = start.line; line <= end.line; line++) {
            auto targetPosition = start;
            targetPosition.line = line;
            if (const auto noteData = locked->noteDataAtPosition(targetPosition); noteData && noteData->type() == NoteData::Type::NoteOn) {
                noteData->setVelocity(static_cast<uint8_t>(interpolator.getValue(targetPosition.line)));
                changedPositions.push_back(targetPosition);
            }
        }
    }
    return changedPositions;
}

} // namespace noteahead
