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
#include "../application/service/automation_service.hpp"
#include "../common/constants.hpp"
#include "../common/utils.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/note_data.hpp"
#include "column_settings.hpp"
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

Pattern::Pattern(size_t index, const PatternConfig & config)
  : m_index { index }
{
    initialize(config);
}

std::unique_ptr<Pattern> Pattern::copyWithoutData(size_t newPatternIndex) const
{
    return std::make_unique<Pattern>(newPatternIndex, patternConfig());
}

Pattern::PatternConfig Pattern::patternConfig() const
{
    PatternConfig config;
    config.lineCount = m_trackOrder.at(0)->lineCount();
    for (auto && track : m_trackOrder) {
        config.trackToColumnCountMap[track->index()] = track->columnCount();
    }
    return config;
}

size_t Pattern::index() const
{
    return m_index;
}

void Pattern::addColumn(size_t trackIndex)
{
    trackByIndexThrow(trackIndex)->addColumn();
}

bool Pattern::deleteColumn(size_t trackIndex)
{
    return trackByIndexThrow(trackIndex)->deleteColumn();
}

size_t Pattern::columnCount(size_t trackIndex) const
{
    return trackByIndexThrow(trackIndex)->columnCount();
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

Pattern::LineList Pattern::lines(const Position & position) const
{
    return trackByIndexThrow(position.track)->lines(position);
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
    return std::ranges::any_of(m_trackOrder, [](auto && track) {
        return track->hasData();
    });
}

bool Pattern::hasData(size_t trackIndex, size_t columnIndex) const
{
    return trackByIndexThrow(trackIndex)->hasData(columnIndex);
}

bool Pattern::hasPosition(const Position & position) const
{
    if (position.pattern != m_index) {
        return false;
    }

    if (const auto track = trackByIndex(position.track); track) {
        return track->hasPosition(position);
    }

    return false;
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
    return trackByIndexThrow(trackIndex)->name();
}

std::string Pattern::columnName(size_t trackIndex, size_t columnIndex) const
{
    return trackByIndexThrow(trackIndex)->columnName(columnIndex);
}

void Pattern::setColumnName(size_t trackIndex, size_t columnIndex, std::string name)
{
    trackByIndexThrow(trackIndex)->setColumnName(columnIndex, name);
}

std::optional<size_t> Pattern::trackByName(std::string_view name) const
{
    const auto track = std::ranges::find_if(m_trackOrder, [=](auto && track) {
        return track->name() == name;
    });
    return track != m_trackOrder.end() ? std::optional<size_t> { (*track)->index() } : std::optional<size_t> {};
}

std::optional<size_t> Pattern::columnByName(size_t trackIndex, std::string_view name) const
{
    if (const auto track = trackByIndex(trackIndex); track) {
        return track->columnByName(name);
    }
    return {};
}

size_t Pattern::nextFreeTrackIndex() const
{
    auto indices = trackIndices();
    std::ranges::sort(indices);
    size_t expectedIndex = 0;
    for (const auto & index : indices) {
        if (index != expectedIndex) {
            return expectedIndex;
        }
        expectedIndex++;
    }
    return expectedIndex;
}

size_t Pattern::maxIndex() const
{
    if (const auto it = std::ranges::max_element(m_trackOrder, [](auto && a, auto && b) { return a->index() < b->index(); }); it != m_trackOrder.end()) {
        return (*it)->index();
    } else {
        return 0;
    }
}

Pattern::TrackS Pattern::trackByIndex(size_t index) const
{
    if (const auto it = std::ranges::find_if(m_trackOrder, [index](auto && track) { return track->index() == index; }); it != m_trackOrder.end()) {
        return *it;
    } else {
        return nullptr;
    }
}

Pattern::TrackS Pattern::trackByIndexThrow(size_t index) const
{
    if (const auto track = trackByIndex(index); track) {
        return track;
    } else {
        juzzlin::L(TAG).error() << "Invalid track index: " << index;
        throw std::runtime_error("Invalid track index: " + std::to_string(index));
    }
}

Pattern::TrackS Pattern::trackByPosition(size_t position) const
{
    return position < m_trackOrder.size() ? m_trackOrder.at(position) : nullptr;
}

Pattern::TrackS Pattern::trackByPositionThrow(size_t position) const
{
    if (const auto track = trackByPosition(position); track) {
        return m_trackOrder.at(position);
    } else {
        juzzlin::L(TAG).error() << "Invalid track position: " << position;
        throw std::runtime_error("Invalid track position: " + std::to_string(position));
    }
}

std::optional<size_t> Pattern::trackPositionByIndex(size_t index) const
{
    if (const auto it = std::ranges::find_if(m_trackOrder, [index](auto && track) { return track->index() == index; }); it != m_trackOrder.end()) {
        return std::distance(m_trackOrder.begin(), it);
    } else {
        return {};
    }
}

std::optional<size_t> Pattern::trackIndexByPosition(size_t trackPosition) const
{
    if (trackPosition <= m_trackOrder.size()) {
        return m_trackOrder.at(trackPosition)->index();
    } else {
        return {};
    }
}

void Pattern::setTrackAtPosition(size_t position, TrackS track)
{
    juzzlin::L(TAG).debug() << "Setting track " << track->name() << " as track " << track->index() << " at position " << position;

    m_trackOrder.at(position) = track;
}

void Pattern::addTrackToRightOf(size_t trackIndex)
{
    if (const auto track = trackPositionByIndex(trackIndex); track.has_value()) {
        juzzlin::L(TAG).debug() << "Add track to the right of track position " << *track;
        const auto newIndex = nextFreeTrackIndex();
        const auto newTrack = std::make_shared<Track>(newIndex, "Track " + std::to_string(newIndex + 1), m_trackOrder.at(0)->lineCount(), 1);
        m_trackOrder.insert(m_trackOrder.begin() + static_cast<long>(*track) + 1, newTrack);
        juzzlin::L(TAG).debug() << "Added track with index " << newIndex << ", new track count: " << m_trackOrder.size();
    } else {
        juzzlin::L(TAG).error() << "Invalid track position: " << *track;
    }
}

void Pattern::deleteTrack(size_t trackIndex)
{
    if (const auto track = trackPositionByIndex(trackIndex); track.has_value()) {
        juzzlin::L(TAG).debug() << "Deleting track at position " << *track;
        m_trackOrder.erase(m_trackOrder.begin() + static_cast<long>(*track));
    } else {
        juzzlin::L(TAG).error() << "Invalid track position: " << *track;
    }
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
    return trackByIndexThrow(trackIndex)->instrument();
}

void Pattern::setInstrument(size_t trackIndex, InstrumentS instrument)
{
    trackByIndexThrow(trackIndex)->setInstrument(instrument);
}

Pattern::ColumnSettingsS Pattern::columnSettings(size_t trackIndex, size_t columnIndex) const
{
    return trackByIndexThrow(trackIndex)->columnSettings(columnIndex);
}

void Pattern::setColumnSettings(size_t trackIndex, size_t columnIndex, ColumnSettingsS settings)
{
    trackByIndexThrow(trackIndex)->setColumnSettings(columnIndex, settings);
}

Pattern::NoteDataS Pattern::noteDataAtPosition(const Position & position) const
{
    return trackByIndexThrow(position.track)->noteDataAtPosition(position);
}

void Pattern::setNoteDataAtPosition(const NoteData & noteData, const Position & position) const
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    trackByIndexThrow(position.track)->setNoteDataAtPosition(noteData, position);
}

Pattern::InstrumentSettingsS Pattern::instrumentSettingsAtPosition(const Position & position) const
{
    return trackByIndexThrow(position.track)->instrumentSettingsAtPosition(position);
}

void Pattern::setInstrumentSettingsAtPosition(const Position & position, InstrumentSettingsS instrumentSettings)
{
    trackByIndexThrow(position.track)->setInstrumentSettingsAtPosition(position, instrumentSettings);
}

Position Pattern::nextNoteDataOnSameColumn(const Position & position) const
{
    return trackByIndexThrow(position.track)->nextNoteDataOnSameColumn(position);
}

Position Pattern::prevNoteDataOnSameColumn(const Position & position) const
{
    return trackByIndexThrow(position.track)->prevNoteDataOnSameColumn(position);
}

Pattern::PositionList Pattern::deleteNoteDataAtPosition(const Position & position)
{
    juzzlin::L(TAG).debug() << "Delete note data at position: " << position.toString();
    return trackByIndexThrow(position.track)->deleteNoteDataAtPosition(position);
}

Pattern::PositionList Pattern::insertNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Insert note data at position: " << noteData.toString() << " @ " << position.toString();
    return trackByIndexThrow(position.track)->insertNoteDataAtPosition(noteData, position);
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
    return trackByIndexThrow(position.track)->transposeTrack(position, semitones);
}

Pattern::PositionList Pattern::transposeColumn(const Position & position, int semitones) const
{
    return trackByIndexThrow(position.track)->transposeColumn(position, semitones);
}

void Pattern::initialize(size_t lineCount, size_t trackCount)
{
    for (size_t i = 0; i < trackCount; i++) {
        m_trackOrder.push_back(std::make_shared<Track>(i, "Track " + std::to_string(i + 1), lineCount, 1));
    }
}

void Pattern::initialize(const PatternConfig & config)
{
    for (auto && [trackIndex, columnCount] : config.trackToColumnCountMap) {
        const auto newTrack = std::make_shared<Track>(trackIndex, "Track " + std::to_string(trackIndex + 1), config.lineCount, 1);
        m_trackOrder.push_back(newTrack);
        while (newTrack->columnCount() < columnCount) {
            newTrack->addColumn();
        }
    }
}

Pattern::EventList Pattern::renderToEvents(AutomationServiceS automationService, size_t startTick, size_t ticksPerLine) const
{
    Pattern::EventList eventList;

    // Note data + events contained by tracks
    for (auto && track : m_trackOrder) {
        const auto trackEvents = track->renderToEvents(startTick, ticksPerLine);
        std::ranges::copy(trackEvents, std::back_inserter(eventList));
    }

    // Automation events from AutomationService
    for (auto && track : m_trackOrder) {
        for (size_t column = 0; column < track->columnCount(); column++) {
            for (size_t line = 0; line < lineCount(); line++) {
                std::ranges::copy(automationService->renderToEventsByColumn(m_index, track->index(), column, startTick, ticksPerLine), std::back_inserter(eventList));
            }
        }
    }

    return eventList;
}

void Pattern::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyPattern());

    writer.writeAttribute(Constants::NahdXml::xmlKeyIndex(), QString::number(m_index));
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(m_name));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLineCount(), QString::number(lineCount()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTrackCount(), QString::number(trackCount()));

    writer.writeStartElement(Constants::NahdXml::xmlKeyTracks());

    std::ranges::for_each(m_trackOrder, [&writer](auto && track) {
        if (track) {
            track->serializeToXml(writer);
        }
    });

    writer.writeEndElement(); // Tracks

    writer.writeEndElement(); // Pattern
}

Pattern::PatternU Pattern::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Pattern started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyIndex());
    const auto name = *Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyName());
    const auto lineCount = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyLineCount());
    const auto trackCount = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyTrackCount());
    auto pattern = std::make_unique<Pattern>(index, lineCount, trackCount);
    pattern->setName(name.toStdString());
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPattern()))) {
        juzzlin::L(TAG).trace() << "Pattern: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTracks())) {
            deserializeTracks(reader, *pattern);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Pattern ended";
    return pattern;
}

void Pattern::deserializeTracks(QXmlStreamReader & reader, Pattern & pattern)
{
    juzzlin::L(TAG).trace() << "Reading Tracks started";
    size_t position = 0;
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTracks()))) {
        juzzlin::L(TAG).trace() << "Tracks: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTrack())) {
            pattern.setTrackAtPosition(position++, Track::deserializeFromXml(reader));
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Tracks ended";
}

} // namespace noteahead
