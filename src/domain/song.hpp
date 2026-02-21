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
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>

#include "../application/position.hpp"
#include "instrument.hpp"
#include "note_data.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class AutomationService;
class Column;
class ColumnSettings;
class CopyManager;
class Event;
class Instrument;
class InstrumentSettings;
class Line;
class LineEvent;
class NoteData;
class Pattern;
class PlayOrder;
class SideChainService;
class Track;

using namespace std::chrono_literals;

class Song
{
public:
    Song();
    ~Song();

    std::chrono::milliseconds autoNoteOffOffset() const;
    void setAutoNoteOffOffset(std::chrono::milliseconds autoNoteOffOffset);
    size_t autoNoteOffOffsetTicks() const;
    size_t autoNoteOffOffsetTicks(std::chrono::milliseconds offset) const;

    using ChangedPositions = std::vector<Position>;
    ChangedPositions cutColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const;
    ChangedPositions copyColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const;
    ChangedPositions pasteColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const;
    NoteChangeList transposeColumn(const Position & position, int semitones) const;

    ChangedPositions cutTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const;
    ChangedPositions copyTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const;
    ChangedPositions pasteTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const;
    NoteChangeList transposeTrack(const Position & position, int semitones) const;

    ChangedPositions cutPattern(size_t patternIndex, CopyManager & copyManager) const;
    ChangedPositions copyPattern(size_t patternIndex, CopyManager & copyManager) const;
    ChangedPositions pastePattern(size_t patternIndex, CopyManager & copyManager) const;
    NoteChangeList transposePattern(const Position & position, int semitones) const;

    using PositionList = std::vector<Position>;
    using PositionListCR = const PositionList &;
    ChangedPositions cutSelection(PositionListCR positions, CopyManager & copyManager) const;
    ChangedPositions copySelection(PositionListCR positions, CopyManager & copyManager) const;
    ChangedPositions pasteSelection(const Position & position, CopyManager & copyManager) const;

    void createPattern(size_t patternIndex);
    bool hasPattern(size_t patternIndex) const;
    bool hasPosition(const Position & position) const;
    bool hasTrack(size_t trackIndex) const;

    void addColumn(size_t trackIndex);
    bool deleteColumn(size_t trackIndex);
    size_t columnCount(size_t trackIndex) const;
    //! For testing purposes as column counts should be consistent over patterns.
    size_t columnCount(size_t patternIndex, size_t trackIndex) const;

    size_t lineCount(size_t patternIndex) const;
    void setLineCount(size_t patternIndex, size_t lineCount);
    using LineS = std::shared_ptr<Line>;
    using LineList = std::vector<LineS>;
    LineList lines(const Position & position) const;

    using PatternS = std::shared_ptr<Pattern>;
    size_t patternCount() const;
    PatternS pattern(size_t patternIndex) const;
    using PatternIndexList = std::vector<size_t>;
    PatternIndexList patternIndices() const;
    size_t patternAtSongPosition(size_t position) const;
    void setPatternAtSongPosition(size_t position, size_t pattern);
    void insertPatternToPlayOrder(size_t position);
    void removePatternFromPlayOrder(size_t position);

    size_t addTrackToRightOf(size_t trackIndex);
    size_t addTrackToLeftOf(size_t trackIndex);
    bool deleteTrack(size_t trackIndex);
    size_t trackCount() const;
    //! For testing purposes as track counts should be consistent over patterns.
    size_t trackCount(size_t patternIndex) const;
    using TrackIndexList = std::vector<size_t>;
    TrackIndexList trackIndices() const;
    std::optional<size_t> trackPositionByIndex(size_t trackIndex) const;
    std::optional<size_t> trackIndexByPosition(size_t trackPosition) const;
    bool isFirstTrack(size_t trackIndex) const;

    bool hasData() const;
    bool hasData(size_t pattern, size_t track, size_t column) const;

    std::string patternName(size_t patternIndex) const;
    void setPatternName(size_t patternIndex, std::string name);
    std::string trackName(size_t trackIndex) const;
    void setTrackName(size_t trackIndex, std::string name);
    std::string columnName(size_t trackIndex, size_t columnIndex) const;
    void setColumnName(size_t trackIndex, size_t columnIndex, std::string name);
    std::optional<size_t> trackByName(std::string_view name) const;
    std::optional<size_t> columnByName(size_t trackIndex, std::string_view name) const;

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(size_t trackIndex) const;
    void setInstrument(size_t trackIndex, InstrumentS instrument);

    using ColumnSettingsS = std::shared_ptr<ColumnSettings>;
    ColumnSettingsS columnSettings(size_t trackIndex, size_t columnIndex) const;
    void setColumnSettings(size_t trackIndex, size_t columnIndex, ColumnSettingsS settings);

    std::string fileName() const;
    void setFileName(std::string fileName);

    using NoteDataS = std::shared_ptr<NoteData>;
    NoteDataS noteDataAtPosition(const Position & position) const;
    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);
    PositionList deleteNoteDataAtPosition(const Position & position);
    PositionList insertNoteDataAtPosition(const NoteData & noteData, const Position & position);

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    using InstrumentSettingsU = std::unique_ptr<InstrumentSettings>;
    InstrumentSettingsS instrumentSettingsAtPosition(const Position & position) const;
    void setInstrumentSettingsAtPosition(const Position & position, InstrumentSettingsS instrumentSettings);

    Position nextNoteDataOnSameColumn(const Position & position) const;
    Position prevNoteDataOnSameColumn(const Position & position) const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using SideChainServiceS = std::shared_ptr<SideChainService>;
    EventList renderToEvents(AutomationServiceS automationService, SideChainServiceS sideChainService, size_t startPosition);
    EventList renderToEvents(AutomationServiceS automationService, SideChainServiceS sideChainService, size_t startPosition, size_t endPosition);

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
    std::chrono::milliseconds duration(size_t startPosition = 0) const;

    size_t length() const;
    void setLength(size_t length);

    //! To decouple MiserService from Song. I don't want Song to know anything about MixerService.
    //! However, concept-wise the mixer settings should still be Song-specific as track configurations may vary.
    //! The same applies to AutomationSerializationCallback.
    using MixerSerializationCallback = std::function<void(QXmlStreamWriter & writer)>;
    using AutomationSerializationCallback = std::function<void(QXmlStreamWriter & writer)>;
    using SideChainSerializationCallback = std::function<void(QXmlStreamWriter & writer)>;
    void serializeToXml(QXmlStreamWriter & writer, MixerSerializationCallback mixerSerializationCallback, AutomationSerializationCallback automationSerializationCallback, SideChainSerializationCallback sideChainSerializationCallback) const;
    void serializeToXmlAsTemplate(QXmlStreamWriter & writer, MixerSerializationCallback mixerSerializationCallback, AutomationSerializationCallback automationSerializationCallback) const;
    using MixerDeserializationCallback = std::function<void(QXmlStreamReader & reader)>;
    using AutomationDeserializationCallback = std::function<void(QXmlStreamReader & reader)>;
    using SideChainDeserializationCallback = std::function<void(QXmlStreamReader & reader)>;
    void deserializeFromXml(QXmlStreamReader & reader, MixerDeserializationCallback mixerDeserializationCallback, AutomationDeserializationCallback automationDeserializationCallback, SideChainDeserializationCallback sideChainDeserializationCallback);

    size_t positionToTick(size_t position) const;
    std::chrono::milliseconds tickToTime(size_t tick) const;
    void updateTickToSongPositionMapping(size_t patternStartTick, size_t songPosition, size_t patternIndex, size_t lineCount);

    void initialize();

private:
    void load(const std::string & filename);

    void deserializePlayOrder(QXmlStreamReader & reader);
    void deserializePosition(QXmlStreamReader & reader);
    void deserializePatterns(QXmlStreamReader & reader);

    using TrackAndColumn = std::pair<int, int>;
    using ActiveNoteMap = std::map<TrackAndColumn, std::set<uint8_t>>;
    using EventListCR = const EventList &;
    EventList applyInstrumentsOnEvents(EventListCR events) const;
    EventList generateNoteOffsForActiveNotes(TrackAndColumn trackAndcolumn, size_t tick, ActiveNoteMap & activeNotes) const;
    EventList generateAutoNoteOffsForDanglingNotes(size_t tick, ActiveNoteMap & activeNotes) const;
    EventList generateChordAutomations(EventListCR events) const;
    EventList generateNoteOffs(EventListCR events) const;
    EventList generateMidiClockEvents(EventListCR eventList, size_t startTick, size_t endTick);
    EventList removeNonMappedNoteOffs(EventListCR events) const;
    EventList renderStartOfSong(size_t tick) const;
    EventList renderEndOfSong(EventListCR eventList, size_t tick) const;

    using EventsAndTick = std::pair<EventList, size_t>;
    EventsAndTick renderPatterns(AutomationServiceS automationService, EventListCR eventList, size_t tick, size_t startPosition, size_t endPosition);
    EventList renderContent(AutomationServiceS automationService, SideChainServiceS sideChainService, size_t startPosition, size_t endPosition);

    std::chrono::milliseconds m_autoNoteOffOffset = 125ms;

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
