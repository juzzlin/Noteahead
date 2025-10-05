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

#include "../application/position.hpp"
#include "../application/service/copy_manager.hpp"
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

Song::ChangedPositions Song::cutSelection(PositionListCR positions, CopyManager & copyManager) const
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

void Song::copySelection(PositionListCR positions, CopyManager & copyManager) const
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
        const auto previousPatternIndex = m_patterns.rbegin()->first;
        juzzlin::L(TAG).debug() << "Copying pattern index=" << patternIndex << " from pattern index=" << previousPatternIndex;
        const auto previousPattern = m_patterns.at(previousPatternIndex);
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

Song::LineList Song::lines(const Position & position) const
{
    return hasPosition(position) ? m_patterns.at(position.pattern)->lines(position) : Song::LineList {};
}

size_t Song::patternCount() const
{
    return static_cast<size_t>(m_patterns.size());
}

Song::PatternIndexList Song::patternIndices() const
{
    Song::PatternIndexList indices;
    for (auto && [index, pattern] : m_patterns) {
        indices.push_back(index);
    }
    return indices;
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

std::optional<size_t> Song::trackIndexByPosition(size_t trackPosition) const
{
    return m_patterns.at(0)->trackIndexByPosition(trackPosition);
}

bool Song::isFirstTrack(size_t trackIndex) const
{
    const auto position = trackPositionByIndex(trackIndex);
    return position.has_value() && position.value() == 0;
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
    return autoNoteOffOffsetTicks(m_autoNoteOffOffset);
}

size_t Song::autoNoteOffOffsetTicks(std::chrono::milliseconds offset) const
{
    const double linesPerMinute = static_cast<double>(m_beatsPerMinute) * static_cast<double>(m_linesPerBeat);
    const double offsetLines = static_cast<double>(offset.count()) * linesPerMinute / 60'000.0;
    const double offsetTicks = offsetLines * static_cast<double>(m_ticksPerLine);
    return static_cast<size_t>(offsetTicks);
}

Song::EventList Song::generateNoteOffsForActiveNotes(TrackAndColumn trackAndColumn, size_t tick, ActiveNoteMap & activeNotes) const
{
    Song::EventList processedEvents;
    if (activeNotes.contains(trackAndColumn)) {
        const auto _activeNotes = activeNotes[trackAndColumn];
        for (auto && activeNote : _activeNotes) {
            NoteData noteData { static_cast<size_t>(trackAndColumn.first), static_cast<size_t>(trackAndColumn.second) };
            noteData.setAsNoteOff(static_cast<uint8_t>(activeNote));
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

Song::EventList Song::generateNoteOffs(EventListCR events) const
{
    Song::EventList processedEvents;
    using TrackAndColumn = std::pair<int, int>;
    std::map<TrackAndColumn, std::set<uint8_t>> activeNotes; // Tracks active notes (key: {track, column}, value: note)

    const auto autoNoteOffOffset = autoNoteOffOffsetTicks();
    juzzlin::L(TAG).info() << "Default auto note-off offset: " << autoNoteOffOffset << " ticks";

    for (const auto & event : events) {
        if (const auto noteData = event->noteData(); noteData) {
            const auto trackAndColumn = std::make_pair(noteData->track(), noteData->column());
            auto instrumentAutoNoteOffOffset = autoNoteOffOffset;
            if (const auto instrument = this->instrument(trackAndColumn.first); instrument) {
                if (instrument->settings().timing.autoNoteOffOffset.has_value()) {
                    instrumentAutoNoteOffOffset = autoNoteOffOffsetTicks(instrument->settings().timing.autoNoteOffOffset.value());
                    juzzlin::L(TAG).info() << "Auto note-off offset override on track" << trackAndColumn.first << ": " << instrumentAutoNoteOffOffset << " ticks";
                }
            }
            if (noteData->type() == NoteData::Type::NoteOn) {
                const auto noteOffEvents = generateNoteOffsForActiveNotes(trackAndColumn, event->tick() - instrumentAutoNoteOffOffset, activeNotes);
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

Song::EventList Song::removeNonMappedNoteOffs(EventListCR events) const
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
    for (auto && pattern : m_playOrder->getPatterns(m_length - startPosition, startPosition)) {
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

Song::EventList Song::applyInstrumentsOnEvents(EventListCR events) const
{
    Song::EventList processedEvents;

    // Find the most negative delay
    std::chrono::milliseconds delayOffset { 0 };
    for (auto && trackIndex : trackIndices()) {
        if (const auto instrument = this->instrument(trackIndex); instrument) {
            delayOffset = std::min(delayOffset, instrument->settings().timing.delay);
        }
    }

    juzzlin::L(TAG).info() << "Delay offset: " << delayOffset.count() << " ms";

    const double msPerTick = 60'000.0 / static_cast<double>(m_beatsPerMinute * m_linesPerBeat * m_ticksPerLine);
    for (auto && event : events) {
        if (event->type() == Event::Type::NoteData) {
            if (const auto noteData = event->noteData(); noteData) {
                event->setInstrument(instrument(noteData->track()));
                if (event->instrument()) {
                    event->applyDelay(event->instrument()->settings().timing.delay - delayOffset, msPerTick);
                    event->applyVelocityJitter(event->instrument()->settings().midiEffects.velocityJitter);
                    event->transpose(event->instrument()->settings().transpose);
                }
            }
        } else if (event->type() == Event::Type::MidiCcData) {
            if (const auto data = event->midiCcData(); data) {
                event->setInstrument(instrument(data->track()));
            }
        } else if (event->type() == Event::Type::PitchBendData) {
            if (const auto data = event->pitchBendData(); data) {
                event->setInstrument(instrument(data->track()));
            }
        } else if (event->type() == Event::Type::InstrumentSettings) {
            if (const auto instrumentSettings = event->instrumentSettings(); instrumentSettings) {
                event->setInstrument(instrument(instrumentSettings->track()));
                if (event->instrument()) {
                    event->applyDelay(event->instrument()->settings().timing.delay - delayOffset, msPerTick);
                }
            }
        }

        processedEvents.push_back(event);
    }

    return processedEvents;
}

Song::EventList Song::renderStartOfSong(size_t tick) const
{
    Song::EventList eventList;
    // Always add an anonymous Start event
    const auto event = std::make_shared<Event>(tick);
    event->setAsStartOfSong();
    eventList.push_back(event);
    // Add Start events per instrument, if enabled
    std::set<QString> processedPortNames;
    for (auto trackIndex : trackIndices()) {
        if (const auto instrument = this->instrument(trackIndex); instrument && instrument->settings().timing.sendTransport) {
            if (const auto portName = instrument->midiAddress().portName(); !processedPortNames.contains(portName)) {
                const auto event = std::make_shared<Event>(tick);
                event->setAsStartOfSong();
                event->setInstrument(instrument);
                eventList.push_back(event);
                processedPortNames.insert(portName);
            }
        }
    }
    return eventList;
}

Song::EventList Song::renderEndOfSong(EventListCR eventList, size_t tick) const
{
    Song::EventList processedEventList { eventList };
    // Always add an anonymous Stop event
    const auto event = std::make_shared<Event>(tick);
    event->setAsEndOfSong();
    processedEventList.push_back(event);
    // Add Stop events per instrument, if enabled
    std::set<QString> processedPortNames;
    for (auto trackIndex : trackIndices()) {
        if (const auto instrument = this->instrument(trackIndex); instrument && instrument->settings().timing.sendTransport) {
            if (const auto portName = instrument->midiAddress().portName(); !processedPortNames.contains(portName)) {
                const auto event = std::make_shared<Event>(tick);
                event->setAsEndOfSong();
                event->setInstrument(instrument);
                processedEventList.push_back(event);
                processedPortNames.insert(portName);
            }
        }
    }
    return processedEventList;
}

Song::EventsAndTick Song::renderPatterns(AutomationServiceS automationService, EventListCR eventList, size_t tick, size_t startPosition, size_t endPosition)
{
    m_tickToSongPositionMap.clear();
    Song::EventList processedEventList { eventList };
    for (size_t songPosition = startPosition; songPosition < m_length && songPosition < endPosition; songPosition++) {
        const auto patternIndex = m_playOrder->positionToPattern(songPosition);
        juzzlin::L(TAG).debug() << "Rendering position " << songPosition << " as pattern " << patternIndex;
        const auto & pattern = m_patterns[patternIndex];
        const auto patternEventList = pattern->renderToEvents(automationService, tick, m_ticksPerLine);
        std::ranges::copy(patternEventList, std::back_inserter(processedEventList));
        updateTickToSongPositionMapping(tick, songPosition, patternIndex, pattern->lineCount());
        tick += pattern->lineCount() * m_ticksPerLine;
    }
    return { processedEventList, tick };
}

Song::EventList Song::generateMidiClockEvents(EventListCR eventList, size_t startTick, size_t endTick)
{
    const size_t midiClockPulsesPerBeat = 24;
    const double ticksPerMidiClock = static_cast<double>(m_ticksPerLine * m_linesPerBeat) / midiClockPulsesPerBeat;
    double currentTick = static_cast<double>(startTick);
    std::set<QString> processedPortNames;
    Song::EventList processedEventList { eventList };
    while (static_cast<size_t>(currentTick) < endTick) {
        processedPortNames.clear();
        for (auto trackIndex : trackIndices()) {
            if (const auto instrument = this->instrument(trackIndex); instrument && instrument->settings().timing.sendMidiClock.has_value() && *instrument->settings().timing.sendMidiClock) {
                if (const auto portName = instrument->midiAddress().portName(); !processedPortNames.contains(portName)) {
                    auto midiClockEvent = std::make_shared<Event>(static_cast<size_t>(currentTick));
                    midiClockEvent->setAsMidiClockOut();
                    midiClockEvent->setInstrument(instrument);
                    processedEventList.push_back(midiClockEvent);
                    processedPortNames.insert(portName);
                }
            }
        }
        currentTick += ticksPerMidiClock;
    }
    return processedEventList;
}

Song::EventList Song::renderContent(AutomationServiceS automationService, size_t startPosition, size_t endPosition)
{
    const size_t startTick = positionToTick(startPosition);
    size_t tick = startTick;

    auto eventList = renderStartOfSong(tick);
    std::tie(eventList, tick) = renderPatterns(automationService, eventList, tick, startPosition, endPosition);

    eventList = renderEndOfSong(eventList, tick);
    eventList = generateNoteOffs(eventList);
    eventList = removeNonMappedNoteOffs(eventList);
    eventList = generateMidiClockEvents(eventList, startTick, tick);

    return eventList;
}

Song::EventList Song::renderToEvents(AutomationServiceS automationService, size_t startPosition)
{
    return renderToEvents(automationService, startPosition, m_length);
}

Song::EventList Song::renderToEvents(AutomationServiceS automationService, size_t startPosition, size_t endPosition)
{
    return applyInstrumentsOnEvents(renderContent(automationService, startPosition, endPosition));
}

void Song::serializeToXml(QXmlStreamWriter & writer, MixerSerializationCallback mixerSerializationCallback, AutomationSerializationCallback automationSerializationCallback) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeySong());

    writer.writeAttribute(Constants::NahdXml::xmlKeyBeatsPerMinute(), QString::number(m_beatsPerMinute));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLinesPerBeat(), QString::number(m_linesPerBeat));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLength(), QString::number(m_length));

    m_playOrder->serializeToXml(writer);

    if (mixerSerializationCallback) {
        mixerSerializationCallback(writer);
    }

    if (automationSerializationCallback) {
        automationSerializationCallback(writer);
    }

    writer.writeStartElement(Constants::NahdXml::xmlKeyPatterns());

    std::ranges::for_each(m_patterns, [&writer](const auto & pattern) {
        if (pattern.second) {
            pattern.second->serializeToXml(writer);
        }
    });

    writer.writeEndElement(); // Patterns
    writer.writeEndElement(); // Song
}

void Song::serializeToXmlAsTemplate(QXmlStreamWriter & writer, MixerSerializationCallback mixerSerializationCallback, AutomationSerializationCallback automationSerializationCallback) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeySong());

    writer.writeAttribute(Constants::NahdXml::xmlKeyBeatsPerMinute(), QString::number(m_beatsPerMinute));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLinesPerBeat(), QString::number(m_linesPerBeat));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLength(), QString::number(1));

    if (mixerSerializationCallback) {
        mixerSerializationCallback(writer);
    }

    if (automationSerializationCallback) {
        automationSerializationCallback(writer);
    }

    writer.writeStartElement(Constants::NahdXml::xmlKeyPatterns());

    m_patterns.at(0)->serializeToXml(writer);

    writer.writeEndElement(); // Patterns
    writer.writeEndElement(); // Song
}

void Song::deserializeFromXml(QXmlStreamReader & reader, MixerDeserializationCallback mixerDeserializationCallback, AutomationDeserializationCallback automationDeserializationCallback)
{
    setBeatsPerMinute(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyBeatsPerMinute()));
    setLinesPerBeat(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyLinesPerBeat()));
    setLength(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyLength()));

    juzzlin::L(TAG).trace() << "Reading Song started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeySong()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::NahdXml::xmlKeyPatterns())) {
                deserializePatterns(reader);
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyPlayOrder())) {
                deserializePlayOrder(reader);
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyMixer())) {
                if (mixerDeserializationCallback) {
                    mixerDeserializationCallback(reader);
                }
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyAutomation())) {
                if (automationDeserializationCallback) {
                    automationDeserializationCallback(reader);
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
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPlayOrder()))) {
        juzzlin::L(TAG).trace() << "PlayOrder: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPosition())) {
            deserializePosition(reader);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading PlayOrder ended";
}

void Song::deserializePosition(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Position started";
    setPatternAtSongPosition(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyIndex()), *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyPatternAttr()));
    juzzlin::L(TAG).trace() << "Reading Position ended";
}

void Song::deserializePatterns(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Patterns started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPatterns()))) {
        juzzlin::L(TAG).trace() << "Patterns: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPattern())) {
            auto pattern = Pattern::deserializeFromXml(reader);
            m_patterns[pattern->index()] = std::move(pattern);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Patterns ended";
}

Song::~Song() = default;

} // namespace noteahead
