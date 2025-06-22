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

#ifndef PLAYER_WORKER_HPP
#define PLAYER_WORKER_HPP

#include <QObject>
#include <cstddef>
#include <memory>
#include <set>
#include <vector>

namespace noteahead {

class Event;
class Instrument;
class InstrumentSettings;
class MidiService;
class MixerService;
class NoteData;

class PlayerWorker : public QObject
{
    Q_OBJECT

public:
    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;

    struct Timing
    {
        quint64 beatsPerMinute = 0;

        quint64 linesPerBeat = 0;

        quint64 ticksPerLine = 0;
    };

    using MidiServiceS = std::shared_ptr<MidiService>;
    using MixerServiceS = std::shared_ptr<MixerService>;
    PlayerWorker(MidiServiceS midiService, MixerServiceS mixerService);

    ~PlayerWorker() override;

    void initialize(const EventList & events, const Timing & timing);

    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    bool isPlaying() const;

    bool isLooping() const;
    void setIsLooping(bool isLooping);

signals:
    void isPlayingChanged();
    void songEnded();
    void tickUpdated(quint64 tick);

private:
    quint64 effectiveTick(quint64 tick, quint64 minTick, quint64 maxTick) const;

    void handleEvent(const Event & event) const;
    void processEvents();

    void setIsPlaying(bool isPlaying);
    bool shouldEventPlay(const Event & event) const;
    void stopAllNotes();
    void stopTransport();

    MidiServiceS m_midiService;
    MixerServiceS m_mixerService;

    Timing m_timing;

    using EventMap = std::unordered_map<quint64, EventList>;
    EventMap m_eventMap;

    using InstrumentS = std::shared_ptr<Instrument>;
    std::set<InstrumentS> m_allInstruments;

    std::atomic_bool m_isPlaying = false;
    std::atomic_bool m_isLooping = false;
};

} // namespace noteahead

#endif // PLAYER_WORKER_HPP
