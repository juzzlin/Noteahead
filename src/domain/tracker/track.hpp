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

namespace noteahead {

class Column;
class Event;
class Instrument;
class InstrumentSettings;
class Line;
class ProjectReader;
class ProjectWriter;
struct Position;
class ColumnSettings;

class Track : public MixerUnit
{
public:
    using ColumnIndexList = std::vector<size_t>;

    Track(size_t index, std::string name, size_t length, size_t columnCount);
    //! Builds the columns with the given indices, in the given order. Used when a pattern is copied.
    Track(size_t index, std::string name, size_t length, const ColumnIndexList & columnIndices);

    using ColumnS = std::shared_ptr<Column>;

    //! Restores the most recently deleted column, or creates a new one if none were deleted.
    void addColumn();
    //! Soft-deletes the last column. Fails on the only remaining column.
    bool deleteColumn();
    //! Soft-deletes the given column: it keeps its data and its index, and only leaves the order.
    bool deleteColumn(size_t columnIndex);
    bool moveColumnLeft(size_t columnIndex);
    bool moveColumnRight(size_t columnIndex);
    void setColumn(ColumnS column);
    std::string columnName(size_t columnIndex) const;
    void setColumnName(size_t columnIndex, std::string name);
    std::optional<size_t> columnByName(std::string_view name) const;

    bool hasColumn(size_t columnIndex) const;
    //! Indices of the live columns, in display order.
    ColumnIndexList columnIndices() const;
    std::optional<size_t> columnPositionByIndex(size_t columnIndex) const;
    std::optional<size_t> columnIndexByPosition(size_t columnPosition) const;
    //! Indices of the soft-deleted columns, oldest first.
    ColumnIndexList deletedColumnIndices() const;

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

    void serializeToXml(ProjectWriter & writer) const;
    using TrackU = std::unique_ptr<Track>;
    static TrackU deserializeFromXml(ProjectReader & reader);

private:
    //! Pan is a track-level lane: MIDI CC 10 applies to the whole channel, so the values written on
    //! a line by the individual columns are averaged into a single event.
    EventList renderPanToEvents(size_t startTick, size_t ticksPerLine) const;

    static void deserializeColumns(ProjectReader & reader, Track & track);

    void initialize(size_t length, size_t columnCount);
    //! Rebuilds the order out of empty columns carrying the given indices. For deserialization only.
    void resetColumnOrder(const ColumnIndexList & columnIndices, size_t length);

    ColumnS columnByIndex(size_t columnIndex) const;
    ColumnS columnByIndexThrow(size_t columnIndex) const;
    size_t nextFreeColumnIndex() const;

    //! Live columns, in display order. A column's index is its identity and does not change: the
    //! automations, the mixer settings and the note data all refer to a column by it.
    std::vector<ColumnS> m_columnOrder;

    //! Soft-deleted columns, most recently deleted last. Deleting a column only moves it here, so
    //! that adding a column back restores its data and everything bound to its index.
    std::vector<ColumnS> m_deletedColumns;

    InstrumentS m_instrument;
};

} // namespace noteahead

#endif // TRACK_HPP
