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

#include "player_worker.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/instrument_settings.hpp"
#include "../domain/note_data.hpp"
#include "midi_service.hpp"

#include <ranges>
#include <thread>

namespace noteahead {

static const auto TAG = "PlayerWorker";

PlayerWorker::PlayerWorker(MidiServiceS midiService)
  : m_midiService { midiService }
{
}

void PlayerWorker::initialize(const EventList & events, const Timing & timing)
{
    if (!m_isPlaying) {
        m_events = events;
        m_timing = timing;
        m_eventMap.clear();
        m_allInstruments.clear();
        for (auto && event : m_events) {
            m_eventMap[event->tick()].push_back(event);
            if (event->instrument()) {
                m_allInstruments.insert(event->instrument());
            }
        }
    } else {
        juzzlin::L(TAG).error() << "Cannot initialize, because still playing!";
    }
}

void PlayerWorker::play()
{
    juzzlin::L(TAG).info() << "Starting playback, event count: " << m_events.size();

    setIsPlaying(true);

    processEvents();
}

void PlayerWorker::stop()
{
    juzzlin::L(TAG).info() << "Stopping playback";

    setIsPlaying(false);
}

bool PlayerWorker::isTrackMuted(size_t trackIndex) const
{
    return m_mutedTracks.contains(trackIndex);
}

bool PlayerWorker::isTrackSoloed(size_t trackIndex) const
{
    return m_soloedTracks.contains(trackIndex);
}

bool PlayerWorker::shouldEventPlay(const Event & event) const
{
    if (m_mutedTracks.empty() && m_soloedTracks.empty()) {
        return true;
    }

    if (auto && noteData = event.noteData(); noteData) {
        const auto trackIndex = noteData->track();
        if (!m_soloedTracks.empty()) {
            return isTrackSoloed(trackIndex) && !isTrackMuted(trackIndex);
        } else {
            return !isTrackMuted(trackIndex);
        }
    }

    return true;
}

void PlayerWorker::handleEvent(const Event & event) const
{
    if (shouldEventPlay(event)) {
        if (auto && noteData = event.noteData(); noteData) {
            juzzlin::L(TAG).trace() << noteData->toString();
            if (auto && instrument = event.instrument(); instrument) {
                if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                    m_midiService->playNote(instrument, *noteData->note(), noteData->velocity());
                } else if (noteData->type() == NoteData::Type::NoteOff) {
                    m_midiService->stopNote(instrument, *noteData->note());
                }
            }
        } else if (auto && instrumentSettings = event.instrumentSettings(); instrumentSettings) {
            juzzlin::L(TAG).trace() << instrumentSettings->toString().toStdString();
            if (auto && instrument = event.instrument(); instrument) {
                auto tempInstrument = *instrument;
                tempInstrument.settings = *instrumentSettings;
                InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyAll, tempInstrument };
                m_midiService->handleInstrumentRequest(instrumentRequest);
            }
        }
    }
}

void PlayerWorker::processEvents()
{
    if (m_eventMap.empty()) {
        juzzlin::L(TAG).debug() << "No events";
        return;
    }

    const auto [minTick, maxTick] = std::ranges::minmax(m_eventMap | std::views::keys);

    juzzlin::L(TAG).debug() << "Min tick: " << minTick;
    juzzlin::L(TAG).debug() << "Max tick: " << maxTick;
    juzzlin::L(TAG).debug() << "Beats per min: " << m_timing.beatsPerMinute;
    juzzlin::L(TAG).debug() << "Lines per beat: " << m_timing.linesPerBeat;
    juzzlin::L(TAG).debug() << "Ticks per line: " << m_timing.ticksPerLine;

    const double tickDurationMs = 60.0 / (static_cast<double>(m_timing.beatsPerMinute * m_timing.linesPerBeat * m_timing.ticksPerLine));
    const auto tickDuration = std::chrono::duration<double> { tickDurationMs };
    const auto startTime = std::chrono::steady_clock::now();

    for (auto tick = minTick; tick <= maxTick && m_isPlaying; tick++) {
        emit tickUpdated(static_cast<size_t>(tick));
        if (auto && eventsAtTick = m_eventMap.find(tick); eventsAtTick != m_eventMap.end()) {
            for (auto && event : eventsAtTick->second) {
                handleEvent(*event);
            }
        }
        // Calculate next tick's start time
        const auto nextTickTime = startTime + std::chrono::duration_cast<std::chrono::steady_clock::duration>((tick - minTick) * tickDuration);
        std::this_thread::sleep_until(nextTickTime);
    }

    juzzlin::L(TAG).debug() << "All events processed";

    setIsPlaying(false);

    juzzlin::L(TAG).debug() << "Stopping all notes";

    stopAllNotes();

    emit songEnded();
}

void PlayerWorker::setIsPlaying(bool isPlaying)
{
    m_isPlaying = isPlaying;

    emit isPlayingChanged();
}

void PlayerWorker::stopAllNotes()
{
    for (auto && instrument : m_allInstruments) {
        m_midiService->stopAllNotes(instrument);
    }
}

bool PlayerWorker::isPlaying() const
{
    return m_isPlaying;
}

void PlayerWorker::muteTrack(size_t trackIndex, bool mute)
{
    juzzlin::L(TAG).debug() << "Muting track " << trackIndex << ": " << mute;
    if (mute) {
        m_mutedTracks.insert(trackIndex);
    } else {
        m_mutedTracks.erase(trackIndex);
    }
}

void PlayerWorker::soloTrack(size_t trackIndex, bool solo)
{
    juzzlin::L(TAG).debug() << "Soloing track " << trackIndex << ": " << solo;
    if (solo) {
        m_soloedTracks.insert(trackIndex);
    } else {
        m_soloedTracks.erase(trackIndex);
    }
}

PlayerWorker::~PlayerWorker()
{
    juzzlin::L(TAG).debug() << "Deleted";
}

} // namespace noteahead
