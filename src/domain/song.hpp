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

#ifndef SONG_HPP
#define SONG_HPP

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>

class QXmlStreamWriter;

namespace cacophony {

class Event;
class Pattern;
struct Position;
class NoteData;

class Song
{
public:
    Song();

    using PatternS = std::shared_ptr<Pattern>;

    uint32_t columnCount(uint32_t trackId) const;

    uint32_t lineCount(uint32_t patternId) const;

    uint32_t patternCount() const;

    uint32_t trackCount() const;

    std::string trackName(uint32_t trackId) const;

    void setTrackName(uint32_t trackId, std::string name);

    std::string fileName() const;

    void setFileName(std::string fileName);

    using NoteDataS = std::shared_ptr<NoteData>;

    NoteDataS noteDataAtPosition(const Position & position) const;

    void setNoteDataAtPosition(const NoteData & noteData, const Position & position);

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

private:
    void initialize();

    EventList introduceNoteOffs(const EventList & events) const;

    void updateTickToPatternAndLineMapping(uint32_t tick, uint32_t patternIndex, uint32_t patternLineCount);

    uint32_t m_beatsPerMinute = 120;

    uint32_t m_linesPerBeat = 8;

    uint32_t m_ticksPerLine = 24;

    std::vector<PatternS> m_patterns;

    std::unordered_map<uint32_t, PatternAndLine> m_tickToPatternAndLineMap;

    std::string m_fileName;
};

} // namespace cacophony

#endif // SONG_HPP
