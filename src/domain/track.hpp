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
class InstrumentSettings;
class NoteData;
struct Position;

class Track
{
public:
    Track(size_t index, std::string name, size_t length, size_t columnCount);

    size_t index() const;

    using ColumnS = std::shared_ptr<Column>;

    void addColumn();
    bool deleteColumn();
    void setColumn(ColumnS column);

    size_t lineCount() const;
    void setLineCount(size_t lineCount);
    size_t columnCount() const;

    bool hasData() const;
    bool hasData(size_t column) const;
    bool hasPosition(const Position & position) const;

    std::string name() const;
    void setName(const std::string & newName);

    using NoteDataS = std::shared_ptr<NoteData>;
    Position nextNoteDataOnSameColumn(const Position & position) const;
    Position prevNoteDataOnSameColumn(const Position & position) const;
    NoteDataS noteDataAtPosition(const Position & position) const;
    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

    using PositionList = std::vector<Position>;
    PositionList deleteNoteDataAtPosition(const Position & position);
    PositionList insertNoteDataAtPosition(const NoteData & noteData, const Position & position);

    PositionList transposeTrack(const Position & position, int semitones) const;
    PositionList transposeColumn(const Position & position, int semitones) const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument() const;
    void setInstrument(InstrumentS instrument);

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    InstrumentSettingsS instrumentSettings(const Position & position) const;
    void setInstrumentSettings(const Position & position, InstrumentSettingsS instrumentSettings);

private:
    void initialize(size_t length, size_t columnCount);

    size_t m_index = 0;

    std::string m_name;

    std::vector<ColumnS> m_columns;

    size_t m_virtualColumnCount = 0;

    InstrumentS m_instrument;
};

} // namespace noteahead

#endif // TRACK_HPP
