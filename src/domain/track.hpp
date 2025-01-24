// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef TRACK_HPP
#define TRACK_HPP

#include <memory>
#include <vector>

class QXmlStreamWriter;

namespace noteahead {

class Column;
class Event;
class Instrument;
class NoteData;
struct Position;

class Track
{
public:
    Track(uint32_t index, std::string name, uint32_t length, uint32_t columnCount);

    uint32_t index() const;

    using ColumnS = std::shared_ptr<Column>;

    void addColumn();

    bool deleteColumn();

    void setColumn(ColumnS column);

    uint32_t lineCount() const;

    void setLineCount(uint32_t lineCount);

    size_t columnCount() const;

    bool hasData() const;

    bool hasData(uint32_t column) const;

    std::string name() const;

    void setName(const std::string & newName);

    using NoteDataS = std::shared_ptr<NoteData>;

    Position nextNoteDataOnSameColumn(const Position & position) const;

    Position prevNoteDataOnSameColumn(const Position & position) const;

    NoteDataS noteDataAtPosition(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument() const;

    void setInstrument(InstrumentS instrument);

private:
    void initialize(uint32_t length, uint32_t columnCount);

    uint32_t m_index = 0;

    std::string m_name;

    std::vector<ColumnS> m_columns;

    size_t m_virtualColumnCount = 0;

    InstrumentS m_instrument;
};

} // namespace noteahead

#endif // TRACK_HPP
