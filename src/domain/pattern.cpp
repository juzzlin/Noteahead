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

#include "pattern.hpp"

#include "../application/position.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/note_data.hpp"
#include "track.hpp"

#include <QXmlStreamWriter>

namespace cacophony {

static const auto TAG = "Pattern";

Pattern::Pattern(uint32_t length, uint32_t trackCount)
{
    initialize(length, trackCount);
}

uint32_t Pattern::columnCount(uint32_t trackId) const
{
    return m_tracks.at(trackId)->columnCount();
}

uint32_t Pattern::lineCount() const
{
    return m_tracks.at(0)->lineCount();
}

uint32_t Pattern::trackCount() const
{
    return static_cast<uint32_t>(m_tracks.size());
}

std::string Pattern::trackName(uint32_t trackId) const
{
    return m_tracks.at(trackId)->name();
}

void Pattern::setTrackName(uint32_t trackId, std::string name)
{
    juzzlin::L(TAG).info() << "Changing name of track " << trackId << " from " << trackName(trackId) << " to " << name;
    m_tracks.at(trackId)->setName(name);
}

Pattern::NoteDataS Pattern::noteDataAtPosition(const Position & position) const
{
    return m_tracks.at(position.track)->noteDataAtPosition(position);
}

void Pattern::setNoteDataAtPosition(const NoteData & noteData, const Position & position) const
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    m_tracks.at(position.track)->setNoteDataAtPosition(noteData, position);
}

void Pattern::initialize(uint32_t length, uint32_t trackCount)
{
    for (uint32_t i = 0; i < trackCount; i++) {
        m_tracks.push_back(std::make_shared<Track>("Track " + std::to_string(i + 1), Track::Type::Drum, length, 1));
    }
}

Pattern::EventList Pattern::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    Pattern::EventList eventList;
    for (auto && track : m_tracks) {
        const auto trackEvents = track->renderToEvents(startTick, ticksPerLine);
        std::copy(trackEvents.begin(), trackEvents.end(), std::back_inserter(eventList));
    }
    return eventList;
}

void Pattern::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement("Pattern");

    writer.writeTextElement("LineCount", QString::number(lineCount()));
    writer.writeTextElement("TrackCount", QString::number(trackCount()));

    writer.writeStartElement("Tracks");
    for (const auto & track : m_tracks) {
        if (track) {
            track->serializeToXml(writer);
        }
    }

    writer.writeEndElement(); // Tracks

    writer.writeEndElement(); // Pattern
}

} // namespace cacophony
