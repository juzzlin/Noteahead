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
#include "../../domain/midi/midi_note_data.hpp"
#include "../../domain/tracker/event.hpp"
#include "../../domain/tracker/instrument_settings.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "jack_service.hpp"
#include "midi_service.hpp"
#include "mixer_service.hpp"

#include <algorithm>
#include <thread>

namespace noteahead {

static const auto TAG = "PlayerWorker";

PlayerWorker::PlayerWorker(MidiServiceS midiService, MixerServiceS mixerService, JackServiceS jackService)
  : m_midiService { midiService }
  , m_mixerService { mixerService }
  , m_jackService { jackService }
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

        // Only a song played entirely by internal devices runs ahead. See m_scheduleLookahead.
        m_everythingInternal = !m_allInstruments.empty()
          && std::all_of(m_allInstruments.begin(), m_allInstruments.end(),
                         [this](auto && instrument) { return m_midiService->isInternalInstrument(instrument); });

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
    if (m_midiService) {
        m_midiService->clearScheduledEvents();
    }
}

std::chrono::steady_clock::duration PlayerWorker::scheduleLookahead() const
{
    return m_scheduleLookahead;
}

void PlayerWorker::resolveScheduleLookahead()
{
    // Settled here rather than at load: it comes from what the backend is doing, and the stream has
    // to have rendered something before it can say. Held for the whole song, since the timeline is
    // anchored on it and moving it mid-song would shift every tick that follows.
    m_scheduleLookahead = std::chrono::steady_clock::duration::zero();
    if (m_everythingInternal && m_midiService) {
        if (const auto lookahead = m_midiService->scheduleLookahead(); lookahead) {
            m_scheduleLookahead = *lookahead;
        }
    }
    juzzlin::L(TAG).info() << "Schedule lookahead: "
                           << std::chrono::duration_cast<std::chrono::milliseconds>(m_scheduleLookahead).count() << " ms";
}

bool PlayerWorker::shouldEventPlay(size_t track, size_t column) const
{
    return m_mixerService->shouldColumnPlay(track, column);
}

void PlayerWorker::handleEvent(const Event & event, std::optional<std::chrono::steady_clock::time_point> when)
{
    event.visit([&](auto && data) {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, NoteData>) {
            if (auto && instrument = event.instrument(); instrument) {
                if (data.type() == NoteData::Type::NoteOff) {
                    // Only what was started is stopped. A column that is muted or left out of the
                    // solo group never had its note-on sent, and one muted while sounding was
                    // already stopped by checkMixerState(), so a note-off here would reach for a
                    // port the song may not even be playing to -- once per note, for every note.
                    auto & notes = m_activeNotes[instrument];
                    const auto sounding = std::erase_if(notes, [&](const ActiveNote & an) {
                        return an.track == data.track() && an.column == data.column() && an.note == *data.note();
                    });
                    if (sounding) {
                        if (when) {
                            m_midiService->stopNoteAt(instrument, { *data.note(), 0 }, *when);
                        } else {
                            m_midiService->stopNote(instrument, { *data.note(), 0 });
                        }
                    }
                } else if (data.type() == NoteData::Type::NoteOn && data.note().has_value()) {
                    if (shouldEventPlay(data.track(), data.column())) {
                        const auto effectiveVelocity = m_mixerService->effectiveVelocity(data.track(), data.column(), data.velocity());
                        if (when) {
                            m_midiService->playNoteAt(instrument, { *data.note(), effectiveVelocity }, *when);
                        } else {
                            m_midiService->playNote(instrument, { *data.note(), effectiveVelocity });
                        }
                        m_activeNotes[instrument].push_back({ data.track(), data.column(), *data.note() });
                    }
                }
            }
        } else if constexpr (std::is_same_v<T, MidiCcData>) {
            if (event.instrument()) {
                m_midiService->sendCcData(event.instrument(), data);
            }
        } else if constexpr (std::is_same_v<T, Event::MidiClockOut>) {
            if (auto && instrument = event.instrument(); instrument && instrument->settings().timing.sendMidiClock.has_value() && *instrument->settings().timing.sendMidiClock) {
                m_midiService->sendClock(instrument);
            }
        } else if constexpr (std::is_same_v<T, Event::StartOfSong>) {
            if (auto && instrument = event.instrument(); instrument && instrument->settings().timing.sendTransport.has_value() && *instrument->settings().timing.sendTransport) {
                juzzlin::L(TAG).info() << "Sending start to " << instrument->midiAddress().portName().toStdString();
                m_midiService->sendStart(instrument);
            }
        } else if constexpr (std::is_same_v<T, Event::EndOfSong>) {
            if (auto && instrument = event.instrument(); instrument && instrument->settings().timing.sendTransport.has_value() && *instrument->settings().timing.sendTransport) {
                juzzlin::L(TAG).info() << "Sending stop to " << instrument->midiAddress().portName().toStdString();
                m_midiService->sendStop(instrument);
            }
        } else if constexpr (std::is_same_v<T, PitchBendData>) {
            if (event.instrument()) {
                m_midiService->sendPitchBendData(event.instrument(), data);
            }
        } else if constexpr (std::is_same_v<T, Event::InstrumentSettingsS>) {
            if (data) {
                juzzlin::L(TAG).trace() << data->toString().toStdString();
                if (auto && instrument = event.instrument(); instrument) {
                    auto tempInstrument = *instrument;
                    tempInstrument.setSettings(*data);
                    InstrumentRequest instrumentRequest { InstrumentRequest::Type::ApplyAll, tempInstrument };
                    m_midiService->handleInstrumentRequest(instrumentRequest);
                }
            }
        }
    });
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

void PlayerWorker::setJackBpmSyncEnabled(bool enabled)
{
    m_jackBpmSyncEnabled = enabled;
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

    resolveScheduleLookahead();

    const auto startTime = std::chrono::steady_clock::now();

    // The whole tick timeline is pushed a lookahead into the future, and the loop then waits until a
    // lookahead before each tick. The waiting is therefore exactly what it always was, and every
    // tick -- the first included -- names a moment that has yet to be rendered. Playback starts that
    // much later, which at a few tens of milliseconds nobody can hear.
    const auto timelineStart = startTime + m_scheduleLookahead;

    const auto timeForTickOffset = [&](quint64 offset) {
        if (m_jackBpmSyncEnabled) {
            const double currentBpm = m_jackService->bpm();
            const double jackSampleRate = m_jackService->sampleRate();
            const double framesPerTick = (jackSampleRate * 60.0) / (currentBpm * m_timing.linesPerBeat * m_timing.ticksPerLine);
            const auto framesToWait = static_cast<long long>(static_cast<double>(offset) * framesPerTick);
            // We sync Noteahead's steady_clock timeline to JACK's sample clock
            return timelineStart + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(framesToWait / jackSampleRate));
        }
        const double tickDurationS = 60.0 / (static_cast<double>(m_timing.beatsPerMinute * m_timing.linesPerBeat * m_timing.ticksPerLine));
        const auto tickDuration = std::chrono::duration<double> { tickDurationS };
        return timelineStart + std::chrono::duration_cast<std::chrono::steady_clock::duration>(offset * tickDuration);
    };

    auto tick = minTick;
    while (m_isPlaying && (tick <= maxTick || m_isLooping)) {
        const auto effectiveTick = this->effectiveTick(tick, minTick, maxTick);
        if (m_timing.ticksPerLine > 0 && effectiveTick % m_timing.ticksPerLine == 0) {
            emit tickUpdated(static_cast<quint64>(effectiveTick));
        }
        if (auto && eventsAtTick = m_eventMap.find(effectiveTick); eventsAtTick != m_eventMap.end()) {
            // The moment this tick is actually due, which is a lookahead from now. Passed on only
            // when the song is running ahead; otherwise the events go out the way they always did.
            const auto due = m_scheduleLookahead.count()
              ? std::optional { timeForTickOffset(tick - minTick) }
              : std::nullopt;
            for (auto && event : eventsAtTick->second) {
                handleEvent(*event, due);
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
        distToLoopEnd = maxTick - effectiveTick + 1;

        step = std::min({ distToNextLine, distToNextEvent, distToLoopEnd });
        if (step < 1) {
            step = 1;
        }

        // When the next tick is due, less the lookahead the timeline was pushed forward by: what is
        // waited for is the same moment as before any of this.
        const auto nextTickTime = timeForTickOffset(tick - minTick + step) - m_scheduleLookahead;

        std::unique_lock<std::mutex> lock { m_mutex };

        const auto spinDuration = std::chrono::milliseconds { 2 };
        const auto sleepTargetTime = nextTickTime - spinDuration;

        while (std::chrono::steady_clock::now() < sleepTargetTime && m_isPlaying) {
            if (m_cv.wait_until(lock, sleepTargetTime, [this] { return m_mixerChanged || !m_isPlaying; })) {
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

        while (std::chrono::steady_clock::now() < nextTickTime && m_isPlaying) {
            // Busy wait for high precision
        }

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
