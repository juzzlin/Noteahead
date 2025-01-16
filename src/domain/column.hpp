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

#ifndef COLUMN_HPP
#define COLUMN_HPP

#include <memory>
#include <vector>

class QXmlStreamWriter;

namespace cacophony {

class Event;
class Line;
class NoteData;
struct Position;

class Column
{
public:
    Column(uint32_t index, uint32_t length);

    uint32_t index() const;

    using NoteDataS = std::shared_ptr<NoteData>;

    bool hasData() const;

    uint32_t lineCount() const;

    void setLineCount(uint32_t lineCount);

    using LineS = std::shared_ptr<Line>;

    void addOrReplaceLine(LineS line);

    Position nextNoteDataOnSameColumn(const Position & position) const;

    Position prevNoteDataOnSameColumn(const Position & position) const;

    NoteDataS noteDataAtPosition(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;

private:
    void initialize(uint32_t length);

    uint32_t m_index = 0;

    std::vector<LineS> m_lines;
};

} // namespace cacophony

#endif // COLUMN_HPP
