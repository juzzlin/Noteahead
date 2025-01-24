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

#ifndef SONG_HPP
#define SONG_HPP

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Column;
class Event;
class Instrument;
class Line;
class Pattern;
struct Position;
class Track;
class NoteData;

class Song
{
public:
    Song();

    void createPattern(uint32_t patternIndex);

    bool hasPattern(uint32_t patternIndex) const;

    void addColumn(uint32_t trackIndex);

    bool deleteColumn(uint32_t trackIndex);

    uint32_t columnCount(uint32_t trackIndex) const;

    //! For testing purposes as column counts should be consistent over patterns.
    uint32_t columnCount(uint32_t patternIndex, uint32_t trackIndex) const;

    uint32_t lineCount(uint32_t patternIndex) const;

    void setLineCount(uint32_t patternIndex, uint32_t lineCount);

    uint32_t patternCount() const;

    uint32_t trackCount() const;

    //! For testing purposes as track counts should be consistent over patterns.
    uint32_t trackCount(uint32_t patternIndex) const;

    bool hasData() const;

    bool hasData(uint32_t pattern, uint32_t track, uint32_t column) const;

    std::string trackName(uint32_t trackIndex) const;

    void setTrackName(uint32_t trackIndex, std::string name);

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(uint32_t trackIndex) const;

    void setInstrument(uint32_t trackIndex, InstrumentS instrument);

    std::string fileName() const;

    void setFileName(std::string fileName);

    using NoteDataS = std::shared_ptr<NoteData>;

    NoteDataS noteDataAtPosition(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

    Position nextNoteDataOnSameColumn(const Position & position) const;

    Position prevNoteDataOnSameColumn(const Position & position) const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents();

    uint32_t beatsPerMinute() const;

    void setBeatsPerMinute(uint32_t bpm);

    uint32_t linesPerBeat() const;

    void setLinesPerBeat(uint32_t lpb);

    uint32_t ticksPerLine() const;

    using PatternAndLine = std::pair<uint32_t, uint32_t>;
    using PatternAndLineOpt = std::optional<PatternAndLine>;
    PatternAndLineOpt patternAndLineByTick(uint32_t tick) const;

    void serializeToXml(QXmlStreamWriter & writer) const;

    void deserializeFromXml(QXmlStreamReader & reader);

    uint32_t autoNoteOffTickOffset() const;

private:
    void load(const std::string & filename);

    void deserializePatterns(QXmlStreamReader & reader);

    using PatternS = std::shared_ptr<Pattern>;

    PatternS deserializePattern(QXmlStreamReader & reader);

    void deserializeTracks(QXmlStreamReader & reader, PatternS pattern);

    using TrackS = std::shared_ptr<Track>;

    TrackS deserializeTrack(QXmlStreamReader & reader);

    void deserializeColumns(QXmlStreamReader & reader, TrackS track);

    using ColumnS = std::shared_ptr<Column>;

    ColumnS deserializeColumn(QXmlStreamReader & reader, uint32_t trackIndex);

    void deserializeLines(QXmlStreamReader & reader, uint32_t trackIndex, ColumnS column);

    using LineS = std::shared_ptr<Line>;

    LineS deserializeLine(QXmlStreamReader & reader, uint32_t trackIndex, uint32_t columnIndex);

    NoteDataS deserializeNoteData(QXmlStreamReader & reader, uint32_t trackIndex, uint32_t columnIndex);

    InstrumentS deserializeInstrument(QXmlStreamReader & reader);

    void initialize();

    void assignInstruments(const EventList & events) const;

    EventList introduceNoteOffs(const EventList & events) const;

    void updateTickToPatternAndLineMapping(size_t tick, size_t patternIndex, size_t patternLineCount);

    uint32_t m_beatsPerMinute = 120;

    uint32_t m_linesPerBeat = 8;

    uint32_t m_ticksPerLine = 24;

    std::map<size_t, PatternS> m_patterns;

    std::unordered_map<size_t, PatternAndLine> m_tickToPatternAndLineMap;

    std::string m_fileName;
};

} // namespace noteahead

#endif // SONG_HPP
