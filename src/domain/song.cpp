// This fi777le is part of Noteahead.
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

#include "song.hpp"

#include "../application/copy_manager.hpp"
#include "../application/position.hpp"
#include "../common/constants.hpp"
#include "../common/utils.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/instrument.hpp"
#include "../domain/instrument_settings.hpp"
#include "../domain/note_data.hpp"
#include "column.hpp"
#include "line.hpp"
#include "line_event.hpp"
#include "pattern.hpp"
#include "play_order.hpp"
#include "track.hpp"

#include <algorithm>
#include <set>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "Song";

Song::Song()
  : m_playOrder { std::make_unique<PlayOrder>() }
{
    initialize();
}

Song::ChangedPositions Song::cutColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const
{
    if (m_patterns.contains(patternIndex)) {
        const auto sourcePattern = m_patterns.at(patternIndex);
        const auto changedPositions = copyManager.pushSourceColumn(*sourcePattern, trackIndex, columnIndex);
        for (auto && changedPosition : changedPositions) {
            sourcePattern->setNoteDataAtPosition(NoteData {}, changedPosition);
        }
        return changedPositions;
    } else {
        return {};
    }
}

void Song::copyColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const
{
    if (m_patterns.contains(patternIndex)) {
        copyManager.pushSourceColumn(*m_patterns.at(patternIndex), trackIndex, columnIndex);
    }
}

Song::ChangedPositions Song::pasteColumn(size_t patternIndex, size_t trackIndex, size_t columnIndex, CopyManager & copyManager) const
{
    return m_patterns.contains(patternIndex) ? copyManager.pasteColumn(m_patterns.at(patternIndex), trackIndex, columnIndex) : Song::ChangedPositions {};
}

Song::ChangedPositions Song::transposeColumn(const Position & position, int semitones) const
{
    return m_patterns.contains(position.pattern) ? m_patterns.at(position.pattern)->transposeColumn(position, semitones) : Song::ChangedPositions {};
}

Song::ChangedPositions Song::cutTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const
{
    if (m_patterns.contains(patternIndex)) {
        const auto sourcePattern = m_patterns.at(patternIndex);
        const auto changedPositions = copyManager.pushSourceTrack(*sourcePattern, trackIndex);
        for (auto && changedPosition : changedPositions) {
            sourcePattern->setNoteDataAtPosition(NoteData {}, changedPosition);
        }
        return changedPositions;
    } else {
        return {};
    }
}

void Song::copyTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const
{
    if (m_patterns.contains(patternIndex)) {
        copyManager.pushSourceTrack(*m_patterns.at(patternIndex), trackIndex);
    }
}

Song::ChangedPositions Song::pasteTrack(size_t patternIndex, size_t trackIndex, CopyManager & copyManager) const
{
    return m_patterns.contains(patternIndex) ? copyManager.pasteTrack(m_patterns.at(patternIndex), trackIndex) : Song::ChangedPositions {};
}

Song::ChangedPositions Song::transposeTrack(const Position & position, int semitones) const
{
    return m_patterns.contains(position.pattern) ? m_patterns.at(position.pattern)->transposeTrack(position, semitones) : Song::ChangedPositions {};
}

Song::ChangedPositions Song::cutPattern(size_t patternIndex, CopyManager & copyManager) const
{
    if (m_patterns.contains(patternIndex)) {
        const auto sourcePattern = m_patterns.at(patternIndex);
        const auto changedPositions = copyManager.pushSourcePattern(*sourcePattern);
        for (auto && changedPosition : changedPositions) {
            sourcePattern->setNoteDataAtPosition(NoteData {}, changedPosition);
        }
        return changedPositions;
    } else {
        return {};
    }
}

void Song::copyPattern(size_t patternIndex, CopyManager & copyManager) const
{
    if (m_patterns.contains(patternIndex)) {
        copyManager.pushSourcePattern(*m_patterns.at(patternIndex));
    }
}

Song::ChangedPositions Song::pastePattern(size_t patternIndex, CopyManager & copyManager) const
{
    return m_patterns.contains(patternIndex) ? copyManager.pastePattern(m_patterns.at(patternIndex)) : Song::ChangedPositions {};
}

Song::ChangedPositions Song::transposePattern(const Position & position, int semitones) const
{
    return m_patterns.contains(position.pattern) ? m_patterns.at(position.pattern)->transposePattern(position, semitones) : Song::ChangedPositions {};
}

Song::ChangedPositions Song::cutSelection(const std::vector<Position> & positions, CopyManager & copyManager) const
{
    if (!positions.empty()) {
        if (const auto patternIndex = positions.at(0).pattern; m_patterns.contains(patternIndex)) {
            const auto sourcePattern = m_patterns.at(patternIndex);
            const auto changedPositions = copyManager.pushSourceSelection(*sourcePattern, positions);
            for (auto && changedPosition : changedPositions) {
                sourcePattern->setNoteDataAtPosition(NoteData {}, changedPosition);
            }
            return changedPositions;
        }
    }
    return {};
}

void Song::copySelection(const std::vector<Position> & positions, CopyManager & copyManager) const
{
    if (!positions.empty()) {
        if (const auto patternIndex = positions.at(0).pattern; m_patterns.contains(patternIndex)) {
            copyManager.pushSourceSelection(*m_patterns.at(patternIndex), positions);
        }
    }
}

Song::ChangedPositions Song::pasteSelection(const Position & position, CopyManager & copyManager) const
{
    return m_patterns.contains(position.pattern) ? copyManager.pasteSelection(m_patterns.at(position.pattern), position) : Song::ChangedPositions {};
}

void Song::createPattern(size_t patternIndex)
{
    if (m_patterns.empty()) {
        initialize();
    } else {
        const auto previousPattern = m_patterns.at(m_patterns.rbegin()->first);
        m_patterns[patternIndex] = previousPattern->copyWithoutData(patternIndex);
    }
}

bool Song::hasPattern(size_t patternIndex) const
{
    return m_patterns.contains(patternIndex);
}

bool Song::hasPosition(const Position & position) const
{
    if (m_patterns.contains(position.pattern)) {
        return m_patterns.at(position.pattern)->hasPosition(position);
    }

    return false;
}

bool Song::hasTrack(size_t trackIndex) const
{
    const auto _trackIndices = trackIndices();
    return std::ranges::find(_trackIndices, trackIndex) != _trackIndices.end();
}

void Song::addColumn(size_t trackIndex)
{
    std::ranges::for_each(m_patterns, [=](const auto & pattern) {
        pattern.second->addColumn(trackIndex);
    });
}

bool Song::deleteColumn(size_t trackIndex)
{
    if (columnCount(trackIndex) > 1) {
        std::ranges::for_each(m_patterns, [=](const auto & pattern) {
            pattern.second->deleteColumn(trackIndex);
        });
        return true;
    }
    return false;
}

size_t Song::columnCount(size_t trackIndex) const
{
    return m_patterns.at(0)->columnCount(trackIndex);
}

size_t Song::columnCount(size_t patternIndex, size_t trackIndex) const
{
    return m_patterns.at(patternIndex)->columnCount(trackIndex);
}

size_t Song::lineCount(size_t patternIndex) const
{
    return m_patterns.at(patternIndex)->lineCount();
}

void Song::setLineCount(size_t patternIndex, size_t lineCount)
{
    m_patterns.at(patternIndex)->setLineCount(lineCount);
}

size_t Song::patternCount() const
{
    return static_cast<size_t>(m_patterns.size());
}

size_t Song::patternAtSongPosition(size_t position) const
{
    return m_playOrder->positionToPattern(position);
}

void Song::setPatternAtSongPosition(size_t position, size_t pattern)
{
    m_playOrder->setPatternAtPosition(position, pattern);
}

void Song::insertPatternToPlayOrder(size_t position)
{
    m_playOrder->insertPattern(position, 0);
}

void Song::removePatternFromPlayOrder(size_t position)
{
    m_playOrder->removePattern(position);
}

void Song::addTrackToRightOf(size_t trackIndex)
{
    std::ranges::for_each(m_patterns, [trackIndex](const auto & pattern) {
        pattern.second->addTrackToRightOf(trackIndex);
    });
}

bool Song::deleteTrack(size_t trackIndex)
{
    if (trackCount(0) > 1) {
        std::ranges::for_each(m_patterns, [trackIndex](const auto & pattern) {
            pattern.second->deleteTrack(trackIndex);
        });
        return true;
    }
    return false;
}

size_t Song::trackCount() const
{
    return m_patterns.at(0)->trackCount();
}

size_t Song::trackCount(size_t patternIndex) const
{
    return m_patterns.at(patternIndex)->trackCount();
}

Song::TrackIndexList Song::trackIndices() const
{
    return m_patterns.at(0)->trackIndices();
}

std::optional<size_t> Song::trackPositionByIndex(size_t trackIndex) const
{
    return m_patterns.at(0)->trackPositionByIndex(trackIndex);
}

std::optional<size_t> Song::trackIndexByPosition(size_t track) const
{
    return m_patterns.at(0)->trackIndexByPosition(track);
}

bool Song::hasData() const
{
    return std::ranges::any_of(m_patterns, [](auto && pattern) {
        return pattern.second->hasData();
    });
}

bool Song::hasData(size_t patternIndex, size_t trackIndex, size_t columnIndex) const
{
    return m_patterns.at(patternIndex)->hasData(trackIndex, columnIndex);
}

std::string Song::patternName(size_t patternIndex) const
{
    return m_patterns.at(patternIndex)->name();
}

void Song::setPatternName(size_t patternIndex, std::string name)
{
    m_patterns.at(patternIndex)->setName(name);
}

std::string Song::trackName(size_t trackIndex) const
{
    return m_patterns.at(0)->trackName(trackIndex);
}

void Song::setTrackName(size_t trackIndex, std::string name)
{
    m_patterns.at(0)->setTrackName(trackIndex, name);
}

Song::InstrumentS Song::instrument(size_t trackIndex) const
{
    return m_patterns.at(0)->instrument(trackIndex);
}

std::string Song::columnName(size_t trackIndex, size_t columnIndex) const
{
    return m_patterns.at(0)->columnName(trackIndex, columnIndex);
}

void Song::setColumnName(size_t trackIndex, size_t columnIndex, std::string name)
{
    return m_patterns.at(0)->setColumnName(trackIndex, columnIndex, name);
}

std::optional<size_t> Song::trackByName(std::string_view name) const
{
    return !name.empty() ? m_patterns.at(0)->trackByName(name) : std::optional<size_t> {};
}

std::optional<size_t> Song::columnByName(size_t trackIndex, std::string_view name) const
{
    return !name.empty() ? m_patterns.at(0)->columnByName(trackIndex, name) : std::optional<size_t> {};
}

void Song::setInstrument(size_t trackIndex, InstrumentS instrument)
{
    m_patterns.at(0)->setInstrument(trackIndex, instrument);
}

Song::InstrumentSettingsS Song::instrumentSettings(const Position & position) const
{
    return m_patterns.at(position.pattern)->instrumentSettings(position);
}

void Song::setInstrumentSettings(const Position & position, InstrumentSettingsS instrumentSettings)
{
    m_patterns.at(position.pattern)->setInstrumentSettings(position, instrumentSettings);
}

std::string Song::fileName() const
{
    return m_fileName;
}

void Song::setFileName(std::string fileName)
{
    m_fileName = fileName;
}

Song::NoteDataS Song::noteDataAtPosition(const Position & position) const
{
    return m_patterns.at(position.pattern)->noteDataAtPosition(position);
}

void Song::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).trace() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    m_patterns.at(position.pattern)->setNoteDataAtPosition(noteData, position);
}

Song::PositionList Song::deleteNoteDataAtPosition(const Position & position)
{
    juzzlin::L(TAG).trace() << "Delete note data at position: " << position.toString();
    return m_patterns.at(position.pattern)->deleteNoteDataAtPosition(position);
}

Song::PositionList Song::insertNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).trace() << "Insert note data at position: " << noteData.toString() << " @ " << position.toString();
    return m_patterns.at(position.pattern)->insertNoteDataAtPosition(noteData, position);
}

Position Song::nextNoteDataOnSameColumn(const Position & position) const
{
    return m_patterns.at(position.pattern)->nextNoteDataOnSameColumn(position);
}

Position Song::prevNoteDataOnSameColumn(const Position & position) const
{
    return m_patterns.at(position.pattern)->prevNoteDataOnSameColumn(position);
}

void Song::initialize()
{
    m_patterns.clear();
    m_patterns[0] = std::make_shared<Pattern>(0, 64, 8);
}

size_t Song::linesPerBeat() const
{
    return m_linesPerBeat;
}

void Song::setLinesPerBeat(size_t lpb)
{
    m_linesPerBeat = lpb;
}

size_t Song::ticksPerLine() const
{
    return m_ticksPerLine;
}

Song::SongPositionOpt Song::songPositionByTick(size_t tick) const
{
    if (const auto iter = m_tickToSongPositionMap.find(tick); iter != m_tickToSongPositionMap.end()) {
        return iter->second;
    } else {
        return {};
    }
}

size_t Song::beatsPerMinute() const
{
    return m_beatsPerMinute;
}

void Song::setBeatsPerMinute(size_t bpm)
{
    m_beatsPerMinute = bpm;
}

std::chrono::milliseconds Song::autoNoteOffOffset() const
{
    return m_autoNoteOffOffset;
}

void Song::setAutoNoteOffOffset(std::chrono::milliseconds autoNoteOffOffset)
{
    m_autoNoteOffOffset = autoNoteOffOffset;
}

size_t Song::autoNoteOffOffsetTicks() const
{
    return static_cast<size_t>(static_cast<size_t>(m_autoNoteOffOffset.count()) * m_beatsPerMinute * m_linesPerBeat / 60 / 1000);
}

Song::EventList Song::generateNoteOffsForActiveNotes(TrackAndColumn trackAndColumn, size_t tick, ActiveNoteMap & activeNotes) const
{
    Song::EventList processedEvents;
    if (activeNotes.contains(trackAndColumn)) {
        const auto _activeNotes = activeNotes[trackAndColumn];
        for (auto && activeNote : _activeNotes) {
            const auto noteData = std::make_shared<NoteData>(trackAndColumn.first, trackAndColumn.second);
            noteData->setAsNoteOff(static_cast<uint8_t>(activeNote));
            activeNotes[trackAndColumn].erase(activeNote);
            processedEvents.push_back(std::make_shared<Event>(tick, noteData));
        }
    }
    return processedEvents;
}

Song::EventList Song::generateAutoNoteOffsForDanglingNotes(size_t tick, ActiveNoteMap & activeNotes) const
{
    Song::EventList processedEvents;
    for (const auto & [trackAndColumn, notes] : activeNotes) {
        const auto noteOffEvents = generateNoteOffsForActiveNotes(trackAndColumn, tick, activeNotes);
        std::ranges::copy(noteOffEvents, std::back_inserter(processedEvents));
    }
    return processedEvents;
}

Song::EventList Song::generateNoteOffs(const EventList & events) const
{
    Song::EventList processedEvents;
    using TrackAndColumn = std::pair<int, int>;
    std::map<TrackAndColumn, std::set<uint8_t>> activeNotes; // Tracks active notes (key: {track, column}, value: note)

    const auto autoNoteOffOffset = this->autoNoteOffOffsetTicks();
    juzzlin::L(TAG).debug() << "Auto note-off offset: " << autoNoteOffOffset << " ticks";

    for (const auto & event : events) {
        if (const auto noteData = event->noteData(); noteData) {
            const auto trackAndColumn = std::make_pair(noteData->track(), noteData->column());
            if (noteData->type() == NoteData::Type::NoteOn) {
                const auto noteOffEvents = generateNoteOffsForActiveNotes(trackAndColumn, event->tick() - autoNoteOffOffset, activeNotes);
                std::ranges::copy(noteOffEvents, std::back_inserter(processedEvents));
                activeNotes[trackAndColumn].insert(*noteData->note());
            } else if (noteData->type() == NoteData::Type::NoteOff) {
                // Map anonymous note-off's to the playing notes
                const auto noteOffEvents = generateNoteOffsForActiveNotes(trackAndColumn, event->tick(), activeNotes);
                std::ranges::copy(noteOffEvents, std::back_inserter(processedEvents));
            }
        }

        processedEvents.push_back(event);
    }

    const auto noteOffEvents = generateAutoNoteOffsForDanglingNotes(events.back()->tick() + 1, activeNotes);
    std::ranges::copy(noteOffEvents, std::back_inserter(processedEvents));

    return processedEvents;
}

Song::EventList Song::removeNonMappedNoteOffs(const EventList & events) const
{
    Song::EventList processedEvents;
    for (const auto & event : events) {
        if (const auto noteData = event->noteData(); noteData) {
            if (noteData->type() != NoteData::Type::NoteOff || noteData->note().has_value()) {
                processedEvents.push_back(event);
            } else {
                juzzlin::L(TAG).debug() << "Skipping non-mapped note-off: " << noteData->toString();
            }
        } else {
            processedEvents.push_back(event);
        }
    }

    return processedEvents;
}

size_t Song::positionToTick(size_t position) const
{
    size_t tick = 0;
    for (size_t i = 0; i < position; i++) {
        tick += m_patterns.at(m_playOrder->positionToPattern(i))->lineCount() * m_ticksPerLine;
    }
    return tick;
}

std::chrono::milliseconds Song::tickToTime(size_t tick) const
{
    return std::chrono::milliseconds { tick * 60'000 / m_beatsPerMinute / m_linesPerBeat / m_ticksPerLine };
}

std::chrono::milliseconds Song::lineToTime(size_t line) const
{
    return tickToTime(line * m_ticksPerLine);
}

std::chrono::milliseconds Song::duration(size_t startPosition) const
{
    std::chrono::milliseconds d {};
    for (auto && pattern : m_playOrder->flatten(m_length - startPosition, startPosition)) {
        d += tickToTime(m_patterns.at(pattern)->lineCount() * m_ticksPerLine);
    }
    return d;
}

size_t Song::length() const
{
    return m_length;
}

void Song::setLength(size_t length)
{
    m_length = length ? length : 1;
}

void Song::updateTickToSongPositionMapping(size_t patternStartTick, size_t songPosition, size_t patternIndex, size_t lineCount)
{
    for (size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
        const auto tick = patternStartTick + lineIndex * m_ticksPerLine;
        const auto time = tickToTime(tick);
        m_tickToSongPositionMap[tick] = { songPosition, patternIndex, lineIndex, time };
    }
}

void Song::assignInstruments(const EventList & events)
{
    // Find the most negative delay
    std::chrono::milliseconds delayOffset { 0 };
    for (auto && trackIndex : trackIndices()) {
        if (const auto instrument = this->instrument(trackIndex); instrument) {
            delayOffset = std::min(delayOffset, instrument->settings().delay);
        }
    }

    juzzlin::L(TAG).info() << "Delay offset: " << delayOffset.count() << " ms";

    const double msPerTick = 60'000.0 / static_cast<double>(m_beatsPerMinute * m_linesPerBeat * m_ticksPerLine);
    std::ranges::for_each(events, [this, delayOffset, msPerTick](const auto & event) {
        if (const auto noteData = event->noteData(); noteData) {
            event->setInstrument(instrument(noteData->track()));
            if (event->instrument()) {
                event->applyDelay(event->instrument()->settings().delay - delayOffset, msPerTick);
            }
        } else if (const auto instrumentSettings = event->instrumentSettings(); instrumentSettings) {
            event->setInstrument(instrument(instrumentSettings->track()));
            if (event->instrument()) {
                event->applyDelay(event->instrument()->settings().delay - delayOffset, msPerTick);
            }
        }
    });
}

Song::EventList Song::renderStartOfSong(size_t tick) const
{
    Song::EventList eventList;
    const auto startOfSongEvent = std::make_shared<Event>(tick);
    startOfSongEvent->setAsStartOfSong();
    eventList.push_back(startOfSongEvent);
    return eventList;
}

Song::EventList Song::renderEndOfSong(Song::EventList eventList, size_t tick) const
{
    const auto endOfSongEvent = std::make_shared<Event>(tick);
    endOfSongEvent->setAsEndOfSong();
    eventList.push_back(endOfSongEvent);
    return eventList;
}

std::pair<Song::EventList, size_t> Song::renderPatterns(Song::EventList eventList, size_t tick, size_t startPosition, size_t endPosition)
{
    m_tickToSongPositionMap.clear();

    for (size_t songPosition = startPosition; songPosition < m_length && songPosition < endPosition; songPosition++) {
        const auto patternIndex = m_playOrder->positionToPattern(songPosition);
        juzzlin::L(TAG).debug() << "Rendering position " << songPosition << " as pattern " << patternIndex;
        const auto & pattern = m_patterns[patternIndex];
        const auto patternEventList = pattern->renderToEvents(tick, m_ticksPerLine);
        std::copy(patternEventList.begin(), patternEventList.end(), std::back_inserter(eventList));
        updateTickToSongPositionMapping(tick, songPosition, patternIndex, pattern->lineCount());
        tick += pattern->lineCount() * m_ticksPerLine;
    }

    return { eventList, tick };
}

Song::EventList Song::generateMidiClockEvents(Song::EventList eventList, size_t startTick, size_t endTick)
{
    const size_t midiClockPulsesPerBeat = 24;
    const double ticksPerMidiClock = static_cast<double>(m_ticksPerLine * m_linesPerBeat) / midiClockPulsesPerBeat;
    double currentTick = static_cast<double>(startTick);
    std::set<QString> processedPortNames;
    while (static_cast<size_t>(currentTick) < endTick) {
        processedPortNames.clear();
        for (auto trackIndex : trackIndices()) {
            if (const auto instrument = this->instrument(trackIndex); instrument && instrument->settings().sendMidiClock.has_value() && *instrument->settings().sendMidiClock) {
                if (const auto portName = instrument->device().portName; !processedPortNames.contains(portName)) {
                    auto midiClockEvent = std::make_shared<Event>(static_cast<size_t>(currentTick));
                    midiClockEvent->setAsMidiClockOut();
                    midiClockEvent->setInstrument(instrument);
                    eventList.push_back(midiClockEvent);
                    processedPortNames.insert(portName);
                }
            }
        }
        currentTick += ticksPerMidiClock;
    }
    return eventList;
}

Song::EventList Song::renderContent(size_t startPosition, size_t endPosition)
{
    const size_t startTick = positionToTick(startPosition);
    size_t tick = startTick;

    auto eventList = renderStartOfSong(tick);

    std::tie(eventList, tick) = renderPatterns(eventList, tick, startPosition, endPosition);

    eventList = renderEndOfSong(eventList, tick);

    eventList = generateNoteOffs(eventList);

    eventList = removeNonMappedNoteOffs(eventList);

    eventList = generateMidiClockEvents(eventList, startTick, tick);

    return eventList;
}

Song::EventList Song::renderToEvents(size_t startPosition)
{
    return renderToEvents(startPosition, m_length);
}

Song::EventList Song::renderToEvents(size_t startPosition, size_t endPosition)
{
    const auto & eventList = renderContent(startPosition, endPosition);
    assignInstruments(eventList);
    return eventList;
}

void Song::serializeToXml(QXmlStreamWriter & writer, MixerSerializationCallback mixerSerializationCallback) const
{
    writer.writeStartElement(Constants::xmlKeySong());

    writer.writeAttribute(Constants::xmlKeyBeatsPerMinute(), QString::number(m_beatsPerMinute));
    writer.writeAttribute(Constants::xmlKeyLinesPerBeat(), QString::number(m_linesPerBeat));
    writer.writeAttribute(Constants::xmlKeyLength(), QString::number(m_length));

    m_playOrder->serializeToXml(writer);

    if (mixerSerializationCallback) {
        mixerSerializationCallback(writer);
    }

    writer.writeStartElement(Constants::xmlKeyPatterns());

    std::ranges::for_each(m_patterns, [&writer](const auto & pattern) {
        if (pattern.second) {
            pattern.second->serializeToXml(writer);
        }
    });

    writer.writeEndElement(); // Patterns
    writer.writeEndElement(); // Song
}

void Song::serializeToXmlAsTemplate(QXmlStreamWriter & writer, MixerSerializationCallback mixerSerializationCallback) const
{
    writer.writeStartElement(Constants::xmlKeySong());

    writer.writeAttribute(Constants::xmlKeyBeatsPerMinute(), QString::number(m_beatsPerMinute));
    writer.writeAttribute(Constants::xmlKeyLinesPerBeat(), QString::number(m_linesPerBeat));
    writer.writeAttribute(Constants::xmlKeyLength(), QString::number(1));

    if (mixerSerializationCallback) {
        mixerSerializationCallback(writer);
    }

    writer.writeStartElement(Constants::xmlKeyPatterns());

    m_patterns.at(0)->serializeToXml(writer);

    writer.writeEndElement(); // Patterns
    writer.writeEndElement(); // Song
}

void Song::deserializeFromXml(QXmlStreamReader & reader, MixerDeserializationCallback mixerDeserializationCallback)
{
    setBeatsPerMinute(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyBeatsPerMinute()));
    setLinesPerBeat(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyLinesPerBeat()));
    setLength(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyLength()));

    juzzlin::L(TAG).trace() << "Reading Song started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeySong()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::xmlKeyPatterns())) {
                deserializePatterns(reader);
            } else if (!reader.name().compare(Constants::xmlKeyPlayOrder())) {
                deserializePlayOrder(reader);
            } else if (!reader.name().compare(Constants::xmlKeyMixer())) {
                if (mixerDeserializationCallback) {
                    mixerDeserializationCallback(reader);
                }
            }
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Song ended";
}

void Song::deserializePlayOrder(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading PlayOrder started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyPlayOrder()))) {
        juzzlin::L(TAG).trace() << "PlayOrder: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyPosition())) {
            deserializePosition(reader);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading PlayOrder ended";
}

void Song::deserializePosition(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Position started";
    setPatternAtSongPosition(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyIndex()), *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyPatternAttr()));
    juzzlin::L(TAG).trace() << "Reading Position ended";
}

void Song::deserializePatterns(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Patterns started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyPatterns()))) {
        juzzlin::L(TAG).trace() << "Patterns: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyPattern())) {
            const auto pattern = deserializePattern(reader);
            m_patterns[pattern->index()] = pattern;
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Patterns ended";
}

Song::PatternS Song::deserializePattern(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Pattern started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto name = *Utils::Xml::readStringAttribute(reader, Constants::xmlKeyName());
    const auto lineCount = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyLineCount());
    const auto trackCount = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyTrackCount());
    const auto pattern = std::make_shared<Pattern>(index, lineCount, trackCount);
    pattern->setName(name.toStdString());
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyPattern()))) {
        juzzlin::L(TAG).trace() << "Pattern: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyTracks())) {
            deserializeTracks(reader, pattern);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Pattern ended";
    return pattern;
}

void Song::deserializeTracks(QXmlStreamReader & reader, PatternS pattern)
{
    juzzlin::L(TAG).trace() << "Reading Tracks started";
    size_t position = 0;
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyTracks()))) {
        juzzlin::L(TAG).trace() << "Tracks: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyTrack())) {
            const auto track = deserializeTrack(reader);
            pattern->setTrackAtPosition(position++, track);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Tracks ended";
}

Song::TrackS Song::deserializeTrack(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Track started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto name = *Utils::Xml::readStringAttribute(reader, Constants::xmlKeyName());
    const auto columnCount = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyColumnCount());
    const auto lineCount = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyLineCount());
    const auto track = std::make_shared<Track>(index, name.toStdString(), lineCount, columnCount);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyTrack()))) {
        juzzlin::L(TAG).trace() << "Track: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyColumns())) {
            deserializeColumns(reader, track);
        } else if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyInstrument())) {
            if (auto instrument = Instrument::deserializeFromXml(reader); instrument) {
                track->setInstrument(std::move(instrument));
            }
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Track ended";
    return track;
}

void Song::deserializeColumns(QXmlStreamReader & reader, TrackS track)
{
    juzzlin::L(TAG).trace() << "Reading Columns started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyColumns()))) {
        juzzlin::L(TAG).trace() << "Columns: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyColumn())) {
            const auto column = deserializeColumn(reader, track->index());
            track->setColumn(column);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Columns ended";
}

Song::ColumnS Song::deserializeColumn(QXmlStreamReader & reader, size_t trackIndex)
{
    juzzlin::L(TAG).trace() << "Reading Column started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto lineCount = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyLineCount());
    const auto column = std::make_shared<Column>(index, lineCount);
    if (const auto name = Utils::Xml::readStringAttribute(reader, Constants::xmlKeyName(), false); name.has_value()) {
        juzzlin::L(TAG).trace() << "Setting column index=" << index << " name to '" << name->toStdString() << "'";
        column->setName(name->toStdString());
    }
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyColumn()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLines())) {
            deserializeLines(reader, trackIndex, column);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Column ended";
    return column;
}

void Song::deserializeLines(QXmlStreamReader & reader, size_t trackIndex, ColumnS column)
{
    juzzlin::L(TAG).trace() << "Reading Lines started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLines()))) {
        juzzlin::L(TAG).trace() << "Deserializing Line: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLine())) {
            const auto line = deserializeLine(reader, trackIndex, column->index());
            column->addOrReplaceLine(line);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Lines ended";
}

Song::LineS Song::deserializeLine(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex)
{
    juzzlin::L(TAG).trace() << "Reading Line started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto line = std::make_shared<Line>(index);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLine()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyNoteData())) {
            line->setNoteData(*deserializeNoteData(reader, trackIndex, columnIndex));
        } else if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLineEvent())) {
            line->setLineEvent(*deserializeLineEvent(reader, trackIndex, columnIndex));
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Line ended";
    return line;
}

Song::LineEventS Song::deserializeLineEvent(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex)
{
    juzzlin::L(TAG).trace() << "Reading LineEvent started";
    const auto lineEvent = std::make_shared<LineEvent>(trackIndex, columnIndex);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLineEvent()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyInstrumentSettings())) {
            if (auto settings = InstrumentSettings::deserializeFromXml(reader); settings) {
                lineEvent->setInstrumentSettings(std::move(settings));
            }
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading LineEvent ended";
    return lineEvent;
}

Song::NoteDataS Song::deserializeNoteData(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex)
{
    juzzlin::L(TAG).trace() << "Reading NoteData";
    const auto typeString = Utils::Xml::readStringAttribute(reader, Constants::xmlKeyType());
    const auto type = typeString == Constants::xmlKeyNoteOn() ? NoteData::Type::NoteOn : NoteData::Type::NoteOff;
    const auto noteData = std::make_shared<NoteData>(trackIndex, columnIndex);
    if (type == NoteData::Type::NoteOn) {
        const auto note = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyNote());
        const auto velocity = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyVelocity());
        noteData->setAsNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity));
    } else {
        noteData->setAsNoteOff();
    }
    return noteData;
}

Song::~Song() = default;

} // namespace noteahead
