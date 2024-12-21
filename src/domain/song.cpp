// This fi777le is part of Cacophony.
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

#include "song.hpp"

#include "../application/position.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/note_data.hpp"
#include "pattern.hpp"

namespace cacophony {

static const auto TAG = "Song";

Song::Song()
{
    initialize();
}

uint32_t Song::columnCount(uint32_t trackId) const
{
    return m_patterns.at(0)->columnCount(trackId);
}

uint32_t Song::lineCount(uint32_t patternId) const
{
    return m_patterns.at(patternId)->lineCount();
}

uint32_t Song::patternCount() const
{
    return static_cast<uint32_t>(m_patterns.size());
}

uint32_t Song::trackCount() const
{
    return m_patterns.at(0)->trackCount();
}

std::string Song::trackName(uint32_t trackId) const
{
    return m_patterns.at(0)->trackName(trackId);
}

void Song::setTrackName(uint32_t trackId, std::string name)
{
    m_patterns.at(0)->setTrackName(trackId, name);
}

Song::NoteDataS Song::noteDataAtPosition(const Position & position) const
{
    return m_patterns.at(position.pattern)->noteDataAtPosition(position);
}

void Song::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    m_patterns.at(position.pattern)->setNoteDataAtPosition(noteData, position);
}

void Song::initialize()
{
    m_patterns.clear();
    m_patterns.push_back(std::make_shared<Pattern>(64, 8));
}

uint32_t Song::linesPerBeat() const
{
    return m_linesPerBeat;
}

void Song::setLinesPerBeat(uint32_t lpb)
{
    m_linesPerBeat = lpb;
}

uint32_t Song::ticksPerLine() const
{
    return m_ticksPerLine;
}

Song::PatternAndLineOpt Song::patternAndLineByTick(uint32_t tick) const
{
    if (const auto iter = m_tickToPatternAndLineMap.find(tick); iter != m_tickToPatternAndLineMap.end()) {
        return iter->second;
    } else {
        return {};
    }
}

uint32_t Song::beatsPerMinute() const
{
    return m_beatsPerMinute;
}

void Song::setBeatsPerMinute(uint32_t bpm)
{
    m_beatsPerMinute = bpm;
}

Song::EventList Song::introduceNoteOffs(const EventList & events) const
{
    Song::EventList processedEvents;
    std::map<std::pair<int, int>, int> activeNotes; // Tracks active notes (key: {track, column}, value: note)

    for (const auto & event : events) {
        if (const auto noteData = event->noteData(); noteData) {
            const auto trackColumn = std::make_pair(noteData->track(), noteData->column());
            if (noteData->type() == NoteData::Type::NoteOn) {
                if (activeNotes.contains(trackColumn)) {
                    const auto activeNote = activeNotes[trackColumn];
                    const auto noteData = std::make_shared<NoteData>(trackColumn.first, trackColumn.second);
                    noteData->setAsNoteOff(activeNote);
                    processedEvents.push_back(std::make_shared<Event>(event->tick() - 1, noteData));
                }
                activeNotes[trackColumn] = noteData->note();
            } else if (noteData->type() == NoteData::Type::NoteOff) {
                activeNotes.erase(trackColumn);
            }
        }

        processedEvents.push_back(event);
    }

    for (const auto & [trackColumn, note] : activeNotes) {
        const auto noteData = std::make_shared<NoteData>(trackColumn.first, trackColumn.second);
        noteData->setAsNoteOff(note);
        processedEvents.push_back(std::make_shared<Event>(events.back()->tick() + 1, noteData));
    }

    return processedEvents;
}

void Song::updateTickToPatternAndLineMapping(uint32_t tick, uint32_t patternIndex, uint32_t patternLineCount)
{
    for (size_t lineIndex = 0; lineIndex < patternLineCount; lineIndex++) {
        m_tickToPatternAndLineMap[tick + lineIndex * m_ticksPerLine] = std::make_pair(patternIndex, lineIndex);
    }
}

Song::EventList Song::renderToEvents()
{
    m_tickToPatternAndLineMap.clear();

    Song::EventList eventList;
    size_t tick = 0;

    const auto startOfSongEvent = std::make_shared<Event>(tick);
    startOfSongEvent->setAsStartOfSong();
    eventList.push_back(startOfSongEvent);

    for (size_t patternIndex = 0; patternIndex < m_patterns.size(); patternIndex++) {
        const auto & pattern = m_patterns[patternIndex];
        const auto patternEventList = pattern->renderToEvents(tick, m_ticksPerLine);
        std::copy(patternEventList.begin(), patternEventList.end(), std::back_inserter(eventList));
        updateTickToPatternAndLineMapping(tick, patternIndex, pattern->lineCount());
        tick += pattern->lineCount() * m_ticksPerLine;
    }

    const auto endOfSongEvent = std::make_shared<Event>(tick);
    endOfSongEvent->setAsEndOfSong();
    eventList.push_back(endOfSongEvent);

    eventList = introduceNoteOffs(eventList);

    return eventList;
}

} // namespace cacophony
