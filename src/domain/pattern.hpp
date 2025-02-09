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

#ifndef PATTERN_HPP
#define PATTERN_HPP

#include <map>
#include <memory>
#include <vector>

class QXmlStreamWriter;

namespace noteahead {

class Event;
class Instrument;
class Track;
class NoteData;
struct Position;

class Pattern
{
public:
    Pattern(size_t index, size_t lineCount, size_t trackCount);

    using ColumnConfig = std::map<size_t, size_t>;

    struct PatternConfig
    {
        size_t lineCount = 0;

        ColumnConfig columnConfig;
    };

    Pattern(size_t index, PatternConfig config);

    size_t index() const;

    void addColumn(size_t trackIndex);

    bool deleteColumn(size_t trackIndex);

    size_t columnCount(size_t trackIndex) const;

    size_t lineCount() const;

    void setLineCount(size_t lineCount);

    size_t trackCount() const;

    bool hasData() const;

    bool hasData(size_t track, size_t column) const;

    bool hasPosition(const Position & position) const;

    std::string name() const;

    void setName(std::string name);

    std::string trackName(size_t trackIndex) const;

    void setTrackName(size_t trackIndex, std::string name);

    using TrackS = std::shared_ptr<Track>;

    void addOrReplaceTrack(TrackS track);

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(size_t trackIndex) const;

    void setInstrument(size_t trackIndex, InstrumentS instrument);

    using NoteDataS = std::shared_ptr<NoteData>;
    NoteDataS noteDataAtPosition(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position) const;

    using PositionList = std::vector<Position>;
    PositionList deleteNoteDataAtPosition(const Position & position);
    PositionList insertNoteDataAtPosition(const NoteData & noteData, const Position & position);

    PositionList transposePattern(const Position & position, int semitones) const;

    PositionList transposeTrack(const Position & position, int semitones) const;

    PositionList transposeColumn(const Position & position, int semitones) const;

    Position nextNoteDataOnSameColumn(const Position & position) const;
    Position prevNoteDataOnSameColumn(const Position & position) const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;

    std::unique_ptr<Pattern> copyWithoutData(size_t index) const;

    PatternConfig patternConfig() const;

private:
    void initialize(size_t lineCount, size_t trackCount);

    size_t m_index = 0;

    std::string m_name;

    std::vector<std::shared_ptr<Track>> m_tracks;
};

} // namespace noteahead

#endif // PATTERN_HPP
