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

#include <algorithm>
#include <thread>

namespace noteahead {

static const auto TAG = "PlayerWorker";

PlayerWorker::PlayerWorker(MidiServiceS midiService, MixerServiceS mixerService)
  : m_midiService { midiService }
  , m_mixerService { mixerService }
{
    connect(m_mixerService.get(), &MixerService::trackMuted, this, &PlayerWorker::onMixerChanged, Qt::DirectConnection);
    connect(m_mixerService.get(), &MixerService::trackSoloed, this, &PlayerWorker::onMixerChanged, Qt::DirectConnection);
    connect(m_mixerService.get(), &MixerService::cleared, this, &PlayerWorker::onMixerChanged, Qt::DirectConnection);
    connect(m_mixerService.get(), &MixerService::configurationChanged, this, &PlayerWorker::onMixerChanged, Qt::DirectConnection);
}

void PlayerWorker::initialize(const EventList & events, const Timing & timing)
{
    juzzlin::L(TAG).info() << "Event count: " << events.size();

    if (!m_isPlaying) {

        m_timing = timing;
        m_eventMap.clear();
        m_allInstruments.clear();
        m_activeNotes.clear();

        for (auto && event : events) {
            m_eventMap[event->tick()].push_back(event);
            if (auto && instrument = event->instrument(); instrument) {
                m_allInstruments.insert(instrument);
            }
        }

    } else {
        juzzlin::L(TAG).error() << "Cannot initialize, because still playing!";
    }
}

void PlayerWorker::onMixerChanged()
{
    std::lock_guard<std::mutex> lock { m_mutex };
    m_mixerChanged = true;
    m_cv.notify_one();
}

void PlayerWorker::checkMixerState()
{
    for (auto & [instrument, notes] : m_activeNotes) {
        std::erase_if(notes, [&](const ActiveNote & activeNote) {
            if (!m_mixerService->shouldColumnPlay(activeNote.track, activeNote.column)) {
                m_midiService->stopNote(instrument, { activeNote.note, 0 });
                return true;
            }
            return false;
        });
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
    m_cv.notify_one(); // Wake up wait loop if sleeping

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
                    auto & notes = m_activeNotes[instrument];
                    std::erase_if(notes, [&](const ActiveNote & an) {
                        return an.track == noteData->track() && an.column == noteData->column() && an.note == *noteData->note();
                    });
                } else if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                    if (shouldEventPlay(noteData->track(), noteData->column())) {
                        const auto effectiveVelocity = m_mixerService->effectiveVelocity(noteData->track(), noteData->column(), noteData->velocity());
                        m_midiService->playNote(instrument, { *noteData->note(), effectiveVelocity });
                        m_activeNotes[instrument].push_back({ noteData->track(), noteData->column(), *noteData->note() });
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

    const auto minTick = m_eventMap.begin()->first;
    const auto maxTick = m_eventMap.rbegin()->first;

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

        quint64 step = 1;

        quint64 distToNextLine = std::numeric_limits<quint64>::max();
        if (m_timing.ticksPerLine > 0) {
            const quint64 rem = effectiveTick % m_timing.ticksPerLine;
            distToNextLine = m_timing.ticksPerLine - rem;
        }

        quint64 distToNextEvent = std::numeric_limits<quint64>::max();
        if (auto it = m_eventMap.upper_bound(effectiveTick); it != m_eventMap.end()) {
            distToNextEvent = it->first - effectiveTick;
        }

        quint64 distToLoopEnd = std::numeric_limits<quint64>::max();
        // If looping, we must wrap around at maxTick.
        // If not looping, we stop at maxTick (effectively), so we don't need to jump beyond it unless empty.
        // But the loop condition is tick <= maxTick.
        // If effectiveTick is near maxTick, distance to wrap is small.
        distToLoopEnd = maxTick - effectiveTick + 1;

        step = std::min({ distToNextLine, distToNextEvent, distToLoopEnd });
        if (step < 1) {
            step = 1;
        }

        // Calculate next tick's start time
        const auto nextTickTime = startTime + std::chrono::duration_cast<std::chrono::steady_clock::duration>((tick - minTick + step) * tickDuration);

        std::unique_lock<std::mutex> lock { m_mutex };
        while (std::chrono::steady_clock::now() < nextTickTime && m_isPlaying) {
            if (m_cv.wait_until(lock, nextTickTime, [this] { return m_mixerChanged || !m_isPlaying; })) {
                if (!m_isPlaying) {
                    break;
                }
                if (m_mixerChanged) {
                    m_mixerChanged = false;
                    lock.unlock();
                    checkMixerState();
                    lock.lock();
                }
            }
        }
        lock.unlock();

        tick += step;
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
