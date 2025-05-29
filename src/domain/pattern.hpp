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
#include <optional>
#include <vector>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class AutomationService;
class Event;
class Instrument;
class InstrumentSettings;
class Line;
class Track;
class NoteData;
struct Position;

class Pattern
{
public:
    Pattern(size_t index, size_t lineCount, size_t trackCount);

    using TrackToColumnCount = std::map<size_t, size_t>;

    struct PatternConfig
    {
        size_t lineCount = 0;

        TrackToColumnCount trackToColumnCountMap;
    };

    Pattern(size_t index, const PatternConfig & config);

    size_t index() const;

    void addColumn(size_t trackIndex);
    bool deleteColumn(size_t trackIndex);
    size_t columnCount(size_t trackIndex) const;

    size_t lineCount() const;
    void setLineCount(size_t lineCount);
    using LineS = std::shared_ptr<Line>;
    using LineList = std::vector<LineS>;
    LineList lines(const Position & position) const;

    size_t trackCount() const;
    using TrackIndexList = std::vector<size_t>;
    TrackIndexList trackIndices() const;
    std::optional<size_t> trackPositionByIndex(size_t trackIndex) const;
    std::optional<size_t> trackIndexByPosition(size_t track) const;

    bool hasData() const;
    bool hasData(size_t trackIndex, size_t columnIndex) const;
    bool hasPosition(const Position & position) const;

    std::string name() const;
    void setName(std::string name);
    std::string trackName(size_t trackIndex) const;
    void setTrackName(size_t trackIndex, std::string name);
    std::string columnName(size_t trackIndex, size_t columnIndex) const;
    void setColumnName(size_t trackIndex, size_t columnIndex, std::string name);
    std::optional<size_t> trackByName(std::string_view name) const;
    std::optional<size_t> columnByName(size_t trackIndex, std::string_view name) const;

    using TrackS = std::shared_ptr<Track>;
    void setTrackAtPosition(size_t position, TrackS track);
    void addTrackToRightOf(size_t trackIndex);
    void deleteTrack(size_t trackIndex);

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(size_t trackIndex) const;
    void setInstrument(size_t trackIndex, InstrumentS instrument);

    using InstrumentSettingsS = std::shared_ptr<InstrumentSettings>;
    InstrumentSettingsS instrumentSettings(const Position & position) const;
    void setInstrumentSettings(const Position & position, InstrumentSettingsS instrumentSettings);

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
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    EventList renderToEvents(AutomationServiceS automationService, size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;
    using PatternU = std::unique_ptr<Pattern>;
    static PatternU deserializeFromXml(QXmlStreamReader & reader);

    std::unique_ptr<Pattern> copyWithoutData(size_t newPatternIndex) const;

    PatternConfig patternConfig() const;

private:
    static void deserializeTracks(QXmlStreamReader & reader, Pattern & pattern);

    void initialize(size_t lineCount, size_t trackCount);
    void initialize(const PatternConfig & config);

    size_t maxIndex() const;
    TrackS trackByIndex(size_t index) const;
    TrackS trackByIndexThrow(size_t index) const;
    TrackS trackByPosition(size_t position) const;
    TrackS trackByPositionThrow(size_t position) const;

    size_t m_index = 0;

    std::string m_name;

    std::vector<TrackS> m_trackOrder;
};

} // namespace noteahead

#endif // PATTERN_HPP
