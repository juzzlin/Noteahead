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
#include <optional>
#include <vector>

#include "mixer_unit.hpp"
#include "note_data.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Column;
class Event;
class Instrument;
class InstrumentSettings;
class Line;
struct Position;
class ColumnSettings;

class Track : public MixerUnit
{
public:
    Track(size_t index, std::string name, size_t length, size_t columnCount);

    using ColumnS = std::shared_ptr<Column>;

    void addColumn();
    bool deleteColumn();
    void setColumn(ColumnS column);
    std::string columnName(size_t columnIndex) const;
    void setColumnName(size_t columnIndex, std::string name);
    std::optional<size_t> columnByName(std::string_view name) const;

    size_t lineCount() const;
    void setLineCount(size_t lineCount);
    using LineS = std::shared_ptr<Line>;
    using LineList = std::vector<LineS>;
    LineList lines(const Position & position) const;
    size_t columnCount() const;

    bool hasData() const;
    bool hasData(size_t column) const;
    bool hasPosition(const Position & position) const;

    using NoteDataS = std::shared_ptr<NoteData>;
    Position nextNoteDataOnSameColumn(const Position & position) const;
    Position prevNoteDataOnSameColumn(const Position & position) const;
    NoteDataS noteDataAtPosition(const Position & position) const;
    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

    using PositionList = std::vector<Position>;
    PositionList deleteNoteDataAtPosition(const Position & position);
    PositionList insertNoteDataAtPosition(const NoteData & noteData, const Position & position);

    NoteChangeList transposeTrack(const Position & position, int semitones) const;
    NoteChangeList transposeColumn(const Position & position, int semitones) const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument() const;
    void setInstrument(InstrumentS instrument);

    using ColumnSettingsS = std::shared_ptr<ColumnSettings>;
    ColumnSettingsS columnSettings(size_t columnIndex) const;
    void setColumnSettings(size_t columnIndex, ColumnSettingsS settings);

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    InstrumentSettingsS instrumentSettingsAtPosition(const Position & position) const;
    void setInstrumentSettingsAtPosition(const Position & position, InstrumentSettingsS instrumentSettings);

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;
    using TrackU = std::unique_ptr<Track>;
    static TrackU deserializeFromXml(QXmlStreamReader & reader);

private:
    static void deserializeColumns(QXmlStreamReader & reader, Track & track);

    void initialize(size_t length, size_t columnCount);

    std::vector<ColumnS> m_columns;

    size_t m_virtualColumnCount = 0;

    InstrumentS m_instrument;
};

} // namespace noteahead

#endif // TRACK_HPP
