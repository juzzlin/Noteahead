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

#include <algorithm>
#include <stdexcept>

namespace noteahead {

static const auto TAG = "Pattern";

Pattern::Pattern(size_t index, size_t lineCount, size_t trackCount)
  : m_index { index }
{
    initialize(lineCount, trackCount);
}

Pattern::Pattern(size_t index, PatternConfig config)
  : m_index { index }
{
    initialize(config.lineCount, config.columnConfig.size());

    std::ranges::for_each(m_trackOrder, [&](auto && track) {
        while (track->columnCount() < config.columnConfig.at(track->index())) {
            track->addColumn();
        }
    });
}

std::unique_ptr<Pattern> Pattern::copyWithoutData(size_t index) const
{
    return std::make_unique<Pattern>(index, patternConfig());
}

Pattern::PatternConfig Pattern::patternConfig() const
{
    PatternConfig config;
    config.lineCount = m_trackOrder.at(0)->lineCount();
    std::ranges::for_each(m_trackOrder, [&](auto && track) {
        config.columnConfig[track->index()] = track->columnCount();
    });
    return config;
}

size_t Pattern::index() const
{
    return m_index;
}

void Pattern::addColumn(size_t trackIndex)
{
    trackByIndex(trackIndex)->addColumn();
}

bool Pattern::deleteColumn(size_t trackIndex)
{
    return trackByIndex(trackIndex)->deleteColumn();
}

size_t Pattern::columnCount(size_t trackIndex) const
{
    return trackByIndex(trackIndex)->columnCount();
}

size_t Pattern::lineCount() const
{
    return m_trackOrder.at(0)->lineCount();
}

void Pattern::setLineCount(size_t lineCount)
{
    for (auto && track : m_trackOrder) {
        track->setLineCount(lineCount);
    }
}

size_t Pattern::trackCount() const
{
    return static_cast<size_t>(m_trackOrder.size());
}

Pattern::TrackIndexList Pattern::trackIndices() const
{
    TrackIndexList indices;
    std::ranges::transform(m_trackOrder, std::back_inserter(indices), [](auto && track) {
        return track->index();
    });
    return indices;
}

bool Pattern::hasData() const
{
    return std::ranges::find_if(m_trackOrder, [](auto && track) {
               return track->hasData();
           })
      != m_trackOrder.end();
}

bool Pattern::hasData(size_t trackIndex, size_t columnIndex) const
{
    return trackByIndex(trackIndex)->hasData(columnIndex);
}

bool Pattern::hasPosition(const Position & position) const
{
    try {
        return position.pattern == m_index && trackByPosition(position.track)->hasPosition(position);
    } catch (...) {
        return false;
    }
}

std::string Pattern::name() const
{
    return m_name;
}

void Pattern::setName(std::string name)
{
    m_name = name;
}

std::string Pattern::trackName(size_t trackIndex) const
{
    return trackByIndex(trackIndex)->name();
}

Pattern::TrackS Pattern::trackByIndex(size_t index) const
{
    if (const auto it = std::ranges::find_if(m_trackOrder, [index](auto && track) { return track->index() == index; }); it != m_trackOrder.end()) {
        return *it;
    } else {
        juzzlin::L(TAG).error() << "Invalid track index:" << index;
        throw std::runtime_error("Invalid track index: " + std::to_string(index));
    }
}

Pattern::TrackS Pattern::trackByPosition(size_t position) const
{
    if (position < m_trackOrder.size()) {
        return m_trackOrder.at(position);
    } else {
        juzzlin::L(TAG).error() << "Invalid track position:" << position;
        throw std::runtime_error("Invalid track position: " + std::to_string(position));
    }
}

void Pattern::addOrReplaceTrack(TrackS track)
{
    juzzlin::L(TAG).debug() << "Setting track " << track->name() << " as track " << track->index();

    m_trackOrder.at(track->index()) = track;
}

void Pattern::setTrackName(size_t trackIndex, std::string name)
{
    juzzlin::L(TAG).debug() << "Changing name of track " << trackIndex << " from " << trackName(trackIndex) << " to " << name;

    if (const auto track = trackByIndex(trackIndex); track) {
        track->setName(name);
    }
}

Pattern::InstrumentS Pattern::instrument(size_t trackIndex) const
{
    return trackByIndex(trackIndex)->instrument();
}

void Pattern::setInstrument(size_t trackIndex, InstrumentS instrument)
{
    trackByIndex(trackIndex)->setInstrument(instrument);
}

Pattern::InstrumentSettingsS Pattern::instrumentSettings(const Position & position) const
{
    return trackByPosition(position.track)->instrumentSettings(position);
}

void Pattern::setInstrumentSettings(const Position & position, InstrumentSettingsS instrumentSettings)
{
    trackByPosition(position.track)->setInstrumentSettings(position, instrumentSettings);
}

Pattern::NoteDataS Pattern::noteDataAtPosition(const Position & position) const
{
    return trackByPosition(position.track)->noteDataAtPosition(position);
}

Position Pattern::nextNoteDataOnSameColumn(const Position & position) const
{
    return trackByPosition(position.track)->nextNoteDataOnSameColumn(position);
}

Position Pattern::prevNoteDataOnSameColumn(const Position & position) const
{
    return trackByPosition(position.track)->prevNoteDataOnSameColumn(position);
}

void Pattern::setNoteDataAtPosition(const NoteData & noteData, const Position & position) const
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    trackByPosition(position.track)->setNoteDataAtPosition(noteData, position);
}

Pattern::PositionList Pattern::deleteNoteDataAtPosition(const Position & position)
{
    juzzlin::L(TAG).debug() << "Delete note data at position: " << position.toString();
    return trackByPosition(position.track)->deleteNoteDataAtPosition(position);
}

Pattern::PositionList Pattern::insertNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Insert note data at position: " << noteData.toString() << " @ " << position.toString();
    return trackByPosition(position.track)->insertNoteDataAtPosition(noteData, position);
}

Pattern::PositionList Pattern::transposePattern(const Position & position, int semitones) const
{
    Pattern::PositionList changedPositions;
    for (size_t track = 0; track < m_trackOrder.size(); track++) {
        auto trackPosition = position;
        trackPosition.track = track; // Need to set track because these positions will be returned back as changed positions.
        std::ranges::copy(m_trackOrder.at(track)->transposeTrack(trackPosition, semitones), std::back_inserter(changedPositions));
    }
    return changedPositions;
}

Pattern::PositionList Pattern::transposeTrack(const Position & position, int semitones) const
{
    return trackByPosition(position.track)->transposeTrack(position, semitones);
}

Pattern::PositionList Pattern::transposeColumn(const Position & position, int semitones) const
{
    return trackByPosition(position.track)->transposeColumn(position, semitones);
}

void Pattern::initialize(size_t lineCount, size_t trackCount)
{
    for (size_t i = 0; i < trackCount; i++) {
        m_trackOrder.push_back(std::make_shared<Track>(i, "Track " + std::to_string(i + 1), lineCount, 1));
    }
}

Pattern::EventList Pattern::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    Pattern::EventList eventList;
    std::ranges::for_each(m_trackOrder, [&](auto && track) {
        const auto trackEvents = track->renderToEvents(startTick, ticksPerLine);
        std::copy(trackEvents.begin(), trackEvents.end(), std::back_inserter(eventList));
    });
    return eventList;
}

void Pattern::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyPattern());

    writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(m_index));
    writer.writeAttribute(Constants::xmlKeyName(), QString::fromStdString(m_name));
    writer.writeAttribute(Constants::xmlKeyLineCount(), QString::number(lineCount()));
    writer.writeAttribute(Constants::xmlKeyTrackCount(), QString::number(trackCount()));

    writer.writeStartElement(Constants::xmlKeyTracks());

    std::ranges::for_each(m_trackOrder, [&writer](auto && track) {
        if (track) {
            track->serializeToXml(writer);
        }
    });

    writer.writeEndElement(); // Tracks

    writer.writeEndElement(); // Pattern
}

} // namespace noteahead
