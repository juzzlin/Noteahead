// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "line.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"

namespace cacophony {

static const auto TAG = "Line";

Line::Line()
  : m_noteData { std::make_unique<NoteData>() }
{
}

Line::Line(const NoteData & noteData)
  : m_noteData { std::make_unique<NoteData>(noteData) }
{
}

void Line::clear()
{
    setNoteData({});
}

void Line::setNoteData(const NoteData & noteData)
{
    juzzlin::L(TAG).debug() << "Set note data " << noteData.toString();
    *m_noteData = noteData;
}

Line::NoteDataS Line::noteData() const
{
    return m_noteData;
}

} // namespace cacophony
