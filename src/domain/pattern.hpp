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

#ifndef PATTERN_HPP
#define PATTERN_HPP

#include <map>
#include <memory>
#include <vector>

class QXmlStreamWriter;

namespace cacophony {

class Event;
class Instrument;
class Track;
class NoteData;
struct Position;

class Pattern
{
public:
    Pattern(uint32_t index, uint32_t lineCount, uint32_t trackCount);

    using ColumnConfig = std::map<uint32_t, uint32_t>;

    struct PatternConfig
    {
        uint32_t lineCount = 0;

        ColumnConfig columnConfig;
    };

    Pattern(uint32_t index, PatternConfig config);

    uint32_t index() const;

    void addColumn(uint32_t trackIndex);

    bool deleteColumn(uint32_t trackIndex);

    uint32_t columnCount(uint32_t trackIndex) const;

    uint32_t lineCount() const;

    void setLineCount(uint32_t lineCount);

    uint32_t trackCount() const;

    bool hasData() const;

    bool hasData(uint32_t track, uint32_t column) const;

    std::string trackName(uint32_t trackIndex) const;

    using TrackS = std::shared_ptr<Track>;

    void addOrReplaceTrack(TrackS track);

    void setTrackName(uint32_t trackIndex, std::string name);

    using InstrumentS = std::shared_ptr<Instrument>;
    InstrumentS instrument(uint32_t trackIndex) const;

    void setInstrument(uint32_t trackIndex, InstrumentS instrument);

    using NoteDataS = std::shared_ptr<NoteData>;
    NoteDataS noteDataAtPosition(const Position & position) const;

    Position nextNoteDataOnSameColumn(const Position & position) const;

    Position prevNoteDataOnSameColumn(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position) const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEvents(size_t startTick, size_t ticksPerLine) const;

    void serializeToXml(QXmlStreamWriter & writer) const;

    std::unique_ptr<Pattern> copyWithoutData(uint32_t index) const;

    PatternConfig patternConfig() const;

private:
    void initialize(uint32_t lineCount, uint32_t trackCount);

    uint32_t m_index = 0;

    std::vector<std::shared_ptr<Track>> m_tracks;
};

} // namespace cacophony

#endif // PATTERN_HPP
