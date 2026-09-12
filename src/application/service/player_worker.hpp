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
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <vector>

namespace noteahead {

class Event;
class Instrument;
class InstrumentSettings;
class JackService;
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
    using JackServiceS = std::shared_ptr<JackService>;
    PlayerWorker(MidiServiceS midiService, MixerServiceS mixerService, JackServiceS jackService);

    ~PlayerWorker() override;

    //! \param deviceSeek Where each port's device belongs when this timeline begins, for a device
    //! that counts the notes it is given. Applied when play starts and again on every loop, so a
    //! looping pattern says the same thing every time round rather than drifting through a lyric.
    using PortNoteCounts = std::map<QString, size_t>;
    void initialize(const EventList & events, const Timing & timing, const PortNoteCounts & deviceSeek = {});

    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    bool isPlaying() const;

    bool isLooping() const;
    void setIsLooping(bool isLooping);

    void setJackBpmSyncEnabled(bool enabled);

signals:
    void isPlayingChanged();
    void songEnded();
    void tickUpdated(quint64 tick);

private:
    quint64 effectiveTick(quint64 tick, quint64 minTick, quint64 maxTick) const;

    void processEvents();

    void setIsPlaying(bool isPlaying);

    void stopAllNotes();
    //! How long @p event's note lasts, in beats, where the render paired it with a note-off.
    std::optional<double> noteBeatsOf(const Event & event) const;
    //! Puts the counting devices back where this timeline begins.
    void seekDevices();
    void stopTransport();

    MidiServiceS m_midiService;
    MixerServiceS m_mixerService;
    JackServiceS m_jackService;

    Timing m_timing;

    using EventMap = std::map<quint64, EventList>;
    EventMap m_eventMap;
    PortNoteCounts m_deviceSeek;

    using InstrumentS = std::shared_ptr<Instrument>;
    std::set<InstrumentS> m_allInstruments;

    //! How far ahead of its own moment a tick is dispatched, so that an internal device can be given
    //! the frame the note belongs on instead of taking whichever block starts next.
    //!
    //! Zero unless every instrument in the song is an internal device. A song that plays anything
    //! out of a port keeps the timing it always had: the port is written from this thread and is
    //! already accurate to the tick, and running ahead without holding those messages back for the
    //! same span would only put the hardware in front of everything else.
    std::chrono::steady_clock::duration m_scheduleLookahead { std::chrono::steady_clock::duration::zero() };
    //! Whether every instrument in the song is played by an internal device, which is what decides
    //! whether the song runs ahead at all.
    bool m_everythingInternal { false };

    struct ActiveNote
    {
        size_t track;
        size_t column;
        quint8 note;
    };

    std::map<InstrumentS, std::vector<ActiveNote>> m_activeNotes;

    std::atomic_bool m_isPlaying = false;
    std::atomic_bool m_isLooping = false;
    std::atomic_bool m_jackBpmSyncEnabled = false;

    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_mixerChanged = false;
private slots:
    void onMixerChanged();

protected:
    void checkMixerState();
    void handleEvent(const Event & event, std::optional<std::chrono::steady_clock::time_point> when = std::nullopt);
    virtual bool shouldEventPlay(size_t track, size_t column) const;
    //! How far ahead of itself the song is being played. See m_scheduleLookahead.
    std::chrono::steady_clock::duration scheduleLookahead() const;
    //! Settles how far ahead this song runs, from what the backend is actually doing.
    void resolveScheduleLookahead();
};

} // namespace noteahead

#endif // PLAYER_WORKER_HPP
