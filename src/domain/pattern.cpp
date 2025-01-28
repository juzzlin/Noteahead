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

#include "pattern.hpp"

#include "../application/position.hpp"
#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/note_data.hpp"
#include "track.hpp"

#include <QXmlStreamWriter>

#include <ranges>

namespace noteahead {

static const auto TAG = "Pattern";

Pattern::Pattern(uint32_t index, uint32_t lineCount, uint32_t trackCount)
  : m_index { index }
{
    initialize(lineCount, trackCount);
}

Pattern::Pattern(uint32_t index, PatternConfig config)
  : m_index { index }
{
    initialize(config.lineCount, config.columnConfig.size());

    std::ranges::for_each(m_tracks, [&](auto && track) {
        while (track->columnCount() < config.columnConfig.at(track->index())) {
            track->addColumn();
        }
    });
}

std::unique_ptr<Pattern> Pattern::copyWithoutData(uint32_t index) const
{
    return std::make_unique<Pattern>(index, patternConfig());
}

Pattern::PatternConfig Pattern::patternConfig() const
{
    PatternConfig config;
    config.lineCount = m_tracks.at(0)->lineCount();
    std::ranges::for_each(m_tracks, [&](auto && track) {
        config.columnConfig[track->index()] = track->columnCount();
    });
    return config;
}

uint32_t Pattern::index() const
{
    return m_index;
}

void Pattern::addColumn(uint32_t trackIndex)
{
    m_tracks.at(trackIndex)->addColumn();
}

bool Pattern::deleteColumn(uint32_t trackIndex)
{
    return m_tracks.at(trackIndex)->deleteColumn();
}

uint32_t Pattern::columnCount(uint32_t trackIndex) const
{
    return m_tracks.at(trackIndex)->columnCount();
}

uint32_t Pattern::lineCount() const
{
    return m_tracks.at(0)->lineCount();
}

void Pattern::setLineCount(uint32_t lineCount)
{
    for (auto && track : m_tracks) {
        track->setLineCount(lineCount);
    }
}

uint32_t Pattern::trackCount() const
{
    return static_cast<uint32_t>(m_tracks.size());
}

bool Pattern::hasData() const
{
    return std::ranges::find_if(m_tracks, [](auto && track) {
               return track->hasData();
           })
      != m_tracks.end();
}

bool Pattern::hasData(uint32_t track, uint32_t column) const
{
    return m_tracks.at(track)->hasData(column);
}

std::string Pattern::trackName(uint32_t trackIndex) const
{
    return m_tracks.at(trackIndex)->name();
}

void Pattern::addOrReplaceTrack(TrackS track)
{
    juzzlin::L(TAG).debug() << "Setting track " << track->name() << " as track " << track->index();

    m_tracks.at(track->index()) = track;
}

void Pattern::setTrackName(uint32_t trackIndex, std::string name)
{
    juzzlin::L(TAG).debug() << "Changing name of track " << trackIndex << " from " << trackName(trackIndex) << " to " << name;

    m_tracks.at(trackIndex)->setName(name);
}

Pattern::InstrumentS Pattern::instrument(uint32_t trackIndex) const
{
    return m_tracks.at(trackIndex)->instrument();
}

void Pattern::setInstrument(uint32_t trackIndex, InstrumentS instrument)
{
    m_tracks.at(trackIndex)->setInstrument(instrument);
}

Pattern::NoteDataS Pattern::noteDataAtPosition(const Position & position) const
{
    return m_tracks.at(position.track)->noteDataAtPosition(position);
}

Position Pattern::nextNoteDataOnSameColumn(const Position & position) const
{
    return m_tracks.at(position.track)->nextNoteDataOnSameColumn(position);
}

Position Pattern::prevNoteDataOnSameColumn(const Position & position) const
{
    return m_tracks.at(position.track)->prevNoteDataOnSameColumn(position);
}

void Pattern::setNoteDataAtPosition(const NoteData & noteData, const Position & position) const
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();

    m_tracks.at(position.track)->setNoteDataAtPosition(noteData, position);
}

void Pattern::initialize(uint32_t lineCount, uint32_t trackCount)
{
    for (uint32_t i = 0; i < trackCount; i++) {
        m_tracks.push_back(std::make_shared<Track>(i, "Track " + std::to_string(i + 1), lineCount, 1));
    }
}

Pattern::EventList Pattern::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    Pattern::EventList eventList;
    std::ranges::for_each(m_tracks, [&](auto && track) {
        const auto trackEvents = track->renderToEvents(startTick, ticksPerLine);
        std::copy(trackEvents.begin(), trackEvents.end(), std::back_inserter(eventList));
    });
    return eventList;
}

void Pattern::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyPattern());

    writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(m_index));
    writer.writeAttribute(Constants::xmlKeyLineCount(), QString::number(lineCount()));
    writer.writeAttribute(Constants::xmlKeyTrackCount(), QString::number(trackCount()));

    writer.writeStartElement(Constants::xmlKeyTracks());

    std::ranges::for_each(m_tracks, [&writer](auto && track) {
        if (track) {
            track->serializeToXml(writer);
        }
    });

    writer.writeEndElement(); // Tracks

    writer.writeEndElement(); // Pattern
}

} // namespace noteahead
