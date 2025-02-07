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

#include <chrono>
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
class CopyManager;
class Event;
class Instrument;
class Line;
class NoteData;
class Pattern;
class PlayOrder;
class Track;
struct Position;

class Song
{
public:
    Song();

    ~Song();

    using ChangedPositions = std::vector<Position>;
    ChangedPositions cutPattern(size_t patternIndex, CopyManager & copyManager) const;

    void copyPattern(size_t patternIndex, CopyManager & copyManager) const;

    ChangedPositions cutTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const;

    void copyTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const;

    ChangedPositions cutColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const;

    void copyColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const;

    ChangedPositions pastePattern(size_t patternIndex, CopyManager & copyManager) const;

    ChangedPositions pasteTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const;

    ChangedPositions pasteColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const;

    ChangedPositions transposePattern(const Position & position, int semitones) const;

    ChangedPositions transposeTrack(const Position & position, int semitones) const;

    ChangedPositions transposeColumn(const Position & position, int semitones) const;

    void createPattern(size_t patternIndex);

    bool hasPattern(size_t patternIndex) const;

    bool hasPosition(const Position & position) const;

    void addColumn(size_t trackIndex);

    bool deleteColumn(size_t trackIndex);

    size_t columnCount(size_t trackIndex) const;

    //! For testing purposes as column counts should be consistent over patterns.
    size_t columnCount(size_t patternIndex, size_t trackIndex) const;

    size_t lineCount(size_t patternIndex) const;

    void setLineCount(size_t patternIndex, size_t lineCount);

    size_t patternCount() const;

    size_t patternAtSongPosition(size_t position) const;

    void setPatternAtSongPosition(size_t position, size_t pattern);

    void insertPatternToPlayOrder(size_t position);

    void removePatternFromPlayOrder(size_t position);

    size_t trackCount() const;

    //! For testing purposes as track counts should be consistent over patterns.
    size_t trackCount(size_t patternIndex) const;

    bool hasData() const;

    bool hasData(size_t pattern, size_t track, size_t column) const;

    std::string patternName(size_t patternIndex) const;

    void setPatternName(size_t patternIndex, std::string name);

    std::string trackName(size_t trackIndex) const;

    void setTrackName(size_t trackIndex, std::string name);

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(size_t trackIndex) const;

    void setInstrument(size_t trackIndex, InstrumentS instrument);

    std::string fileName() const;

    void setFileName(std::string fileName);

    using NoteDataS = std::shared_ptr<NoteData>;

    NoteDataS noteDataAtPosition(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

    using PositionList = std::vector<Position>;
    PositionList deleteNoteDataAtPosition(const NoteData & noteData, const Position & position);
    PositionList insertNoteDataAtPosition(const NoteData & noteData, const Position & position);

    Position nextNoteDataOnSameColumn(const Position & position) const;

    Position prevNoteDataOnSameColumn(const Position & position) const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startPosition);

    size_t beatsPerMinute() const;

    void setBeatsPerMinute(size_t bpm);

    size_t linesPerBeat() const;

    void setLinesPerBeat(size_t lpb);

    size_t ticksPerLine() const;

    struct SongPosition
    {
        size_t position = 0;

        size_t pattern = 0;

        size_t line = 0;

        std::chrono::milliseconds currentTime;
    };
    using SongPositionOpt = std::optional<SongPosition>;
    SongPositionOpt songPositionByTick(size_t tick) const;

    std::chrono::milliseconds lineToTime(size_t line) const;

    std::chrono::milliseconds duration() const;

    size_t length() const;

    void setLength(size_t length);

    void serializeToXml(QXmlStreamWriter & writer) const;

    void deserializeFromXml(QXmlStreamReader & reader);

    size_t autoNoteOffTickOffset() const;

private:
    void load(const std::string & filename);

    void deserializePlayOrder(QXmlStreamReader & reader);

    void deserializePosition(QXmlStreamReader & reader);

    void deserializePatterns(QXmlStreamReader & reader);

    using PatternS = std::shared_ptr<Pattern>;

    PatternS deserializePattern(QXmlStreamReader & reader);

    void deserializeTracks(QXmlStreamReader & reader, PatternS pattern);

    using TrackS = std::shared_ptr<Track>;

    TrackS deserializeTrack(QXmlStreamReader & reader);

    void deserializeColumns(QXmlStreamReader & reader, TrackS track);

    using ColumnS = std::shared_ptr<Column>;

    ColumnS deserializeColumn(QXmlStreamReader & reader, size_t trackIndex);

    void deserializeLines(QXmlStreamReader & reader, size_t trackIndex, ColumnS column);

    using LineS = std::shared_ptr<Line>;

    LineS deserializeLine(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex);

    NoteDataS deserializeNoteData(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex);

    InstrumentS deserializeInstrument(QXmlStreamReader & reader);

    void initialize();

    void assignInstruments(const EventList & events) const;

    EventList introduceNoteOffs(const EventList & events) const;

    EventList renderStartOfSong(size_t tick) const;

    EventList renderEndOfSong(Song::EventList eventList, size_t tick) const;

    std::pair<Song::EventList, size_t> renderPatterns(Song::EventList eventList, size_t tick, size_t startPosition);

    EventList renderContent(size_t startPosition);

    size_t startPositionToTick(size_t startPosition) const;

    std::chrono::milliseconds tickToTime(size_t tick) const;

    void updateTickToSongPositionMapping(size_t patternStartTick, size_t songPosition, size_t patternIndex, size_t lineCount);

    size_t m_beatsPerMinute = 120;

    size_t m_linesPerBeat = 8;

    size_t m_ticksPerLine = 24;

    std::map<size_t, PatternS> m_patterns;

    std::unique_ptr<PlayOrder> m_playOrder;

    size_t m_length = 1;

    std::unordered_map<size_t, SongPosition> m_tickToSongPositionMap;

    std::string m_fileName;
};

} // namespace noteahead

#endif // SONG_HPP
