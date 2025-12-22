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

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/event.hpp"
#include "../../domain/instrument_settings.hpp"
#include "../../domain/midi_note_data.hpp"
#include "../../domain/note_data.hpp"
#include "midi_service.hpp"
#include "mixer_service.hpp"

#include <ranges>
#include <thread>

namespace noteahead {

static const auto TAG = "PlayerWorker";

PlayerWorker::PlayerWorker(MidiServiceS midiService, MixerServiceS mixerService)
  : m_midiService { midiService }
  , m_mixerService { mixerService }
{
}

void PlayerWorker::initialize(const EventList & events, const Timing & timing)
{
    juzzlin::L(TAG).info() << "Event count: " << events.size();

    if (!m_isPlaying) {

        m_timing = timing;
        m_eventMap.clear();
        m_allInstruments.clear();
        m_stopEventSentOnTrack.clear();

        for (auto && event : events) {
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
    juzzlin::L(TAG).info() << "Starting playback";

    setIsPlaying(true);

    processEvents();
}

void PlayerWorker::stop()
{
    juzzlin::L(TAG).info() << "Stopping playback";

    setIsPlaying(false);

    stopAllNotes();
    stopTransport();
}

bool PlayerWorker::shouldEventPlay(size_t track, size_t column) const
{
    return m_mixerService->shouldColumnPlay(track, column);
}

void PlayerWorker::handleEvent(const Event & event)
{
    if (event.type() == Event::Type::NoteData) {
        if (auto && noteData = event.noteData(); noteData) {
            if (auto && instrument = event.instrument(); instrument) {
                if (noteData->type() == NoteData::Type::NoteOff) {
                    m_midiService->stopNote(instrument, { *noteData->note(), 0 });
                } else if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                    if (shouldEventPlay(noteData->track(), noteData->column())) {
                        m_stopEventSentOnTrack.erase(noteData->track());
                        const auto effectiveVelocity = m_mixerService->effectiveVelocity(noteData->track(), noteData->column(), noteData->velocity());
                        m_midiService->playNote(instrument, { *noteData->note(), effectiveVelocity });
                    } else {
                        // When user has muted a track (or soloed some other track) we need
                        // to also stop possibly playing notes on the corresponding channel.
                        // However, we must not do this if only a specific column is muted.
                        if (!m_mixerService->shouldTrackPlay(noteData->track())) {
                            if (!m_stopEventSentOnTrack.contains(noteData->track())) {
                                m_midiService->stopAllNotes(instrument);
                                m_stopEventSentOnTrack.insert(noteData->track());
                            }
                        }
                    }
                }
            }
        }
    } else if (event.type() == Event::Type::MidiCcData) {
        if (event.midiCcData() && event.instrument()) {
            m_midiService->sendCcData(event.instrument(), *event.midiCcData());
        }
    } else if (event.type() == Event::Type::MidiClockOut) {
        // There should not be any clock events generated if disabled, but double-checking won't make any harm
        if (auto && instrument = event.instrument(); instrument && instrument->settings().timing.sendMidiClock.has_value() && *instrument->settings().timing.sendMidiClock) {
            m_midiService->sendClock(instrument);
        }
    } else if (event.type() == Event::Type::StartOfSong) {
        if (auto && instrument = event.instrument(); instrument && instrument->settings().timing.sendTransport.has_value() && *instrument->settings().timing.sendTransport) {
            juzzlin::L(TAG).info() << "Sending start to " << instrument->midiAddress().portName().toStdString();
            m_midiService->sendStart(instrument);
        }
    } else if (event.type() == Event::Type::EndOfSong) {
        if (auto && instrument = event.instrument(); instrument && instrument->settings().timing.sendTransport.has_value() && *instrument->settings().timing.sendTransport) {
            juzzlin::L(TAG).info() << "Sending stop to " << instrument->midiAddress().portName().toStdString();
            m_midiService->sendStop(instrument);
        }
    } else if (event.type() == Event::Type::PitchBendData) {
        if (event.pitchBendData() && event.instrument()) {
            m_midiService->sendPitchBendData(event.instrument(), *event.pitchBendData());
        }
    } else if (event.type() == Event::Type::InstrumentSettings) {
        if (auto && instrumentSettings = event.instrumentSettings(); instrumentSettings) {
            juzzlin::L(TAG).trace() << instrumentSettings->toString().toStdString();
            if (auto && instrument = event.instrument(); instrument) {
                auto tempInstrument = *instrument;
                tempInstrument.setSettings(*instrumentSettings);
                InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyAll, tempInstrument };
                m_midiService->handleInstrumentRequest(instrumentRequest);
            }
        }
    }
}

quint64 PlayerWorker::effectiveTick(quint64 tick, quint64 minTick, quint64 maxTick) const
{
    if (m_isLooping) {
        if (const quint64 range = maxTick - minTick + 1; range == 0) {
            return minTick; // Avoid division by zero, return a default value
        } else {
            return minTick + (tick - minTick) % range;
        }
    } else {
        return tick;
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

    const double tickDurationS = 60.0 / (static_cast<double>(m_timing.beatsPerMinute * m_timing.linesPerBeat * m_timing.ticksPerLine));
    const auto tickDuration = std::chrono::duration<double> { tickDurationS };
    const auto startTime = std::chrono::steady_clock::now();

    auto tick = minTick;
    while (m_isPlaying && (tick <= maxTick || m_isLooping)) {
        const auto effectiveTick = this->effectiveTick(tick, minTick, maxTick);
        if (m_timing.ticksPerLine > 0 && effectiveTick % m_timing.ticksPerLine == 0) {
            emit tickUpdated(static_cast<quint64>(effectiveTick));
        }
        if (auto && eventsAtTick = m_eventMap.find(effectiveTick); eventsAtTick != m_eventMap.end()) {
            for (auto && event : eventsAtTick->second) {
                handleEvent(*event);
            }
        }
        // Calculate next tick's start time
        const auto nextTickTime = startTime + std::chrono::duration_cast<std::chrono::steady_clock::duration>((tick - minTick + 1) * tickDuration);
        std::this_thread::sleep_until(nextTickTime);
        tick++;
    }

    juzzlin::L(TAG).debug() << "All events processed";

    stop();

    emit songEnded();
}

void PlayerWorker::setIsPlaying(bool isPlaying)
{
    m_isPlaying = isPlaying;

    emit isPlayingChanged();
}

void PlayerWorker::stopAllNotes()
{
    juzzlin::L(TAG).info() << "Stopping all notes";
    for (auto && instrument : m_allInstruments) {
        m_midiService->stopAllNotes(instrument);
    }
}

void PlayerWorker::stopTransport()
{
    juzzlin::L(TAG).info() << "Stopping transport";
    for (auto && instrument : m_allInstruments) {
        if (instrument->settings().timing.sendTransport.has_value() && *instrument->settings().timing.sendTransport) {
            juzzlin::L(TAG).info() << "Sending stop to " << instrument->midiAddress().portName().toStdString();
            m_midiService->sendStop(instrument);
        }
    }
}

bool PlayerWorker::isPlaying() const
{
    return m_isPlaying;
}

bool PlayerWorker::isLooping() const
{
    return m_isLooping;
}

void PlayerWorker::setIsLooping(bool isLooping)
{
    m_isLooping = isLooping;
}

PlayerWorker::~PlayerWorker()
{
    juzzlin::L(TAG).debug() << "Deleted";
}

} // namespace noteahead
