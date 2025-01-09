// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef PLAYER_WORKER_HPP
#define PLAYER_WORKER_HPP

#include <QObject>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace cacophony {

class Event;

class PlayerWorker : public QObject
{
    Q_OBJECT

public:
    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;

    struct Timing
    {
        uint32_t beatsPerMinute = 0;

        uint32_t linesPerBeat = 0;

        uint32_t ticksPerLine = 0;
    };

    explicit PlayerWorker(const EventList & events, const Timing & timing);

    ~PlayerWorker() override;

    Q_INVOKABLE void play();

    Q_INVOKABLE void stop();

signals:
    void songEnded();

    void tickUpdated(uint32_t tick);

private:
    void processEvents();

    EventList m_events;

    Timing m_timing;

    using EventMap = std::unordered_map<size_t, EventList>;
    EventMap m_eventMap;

    std::atomic_bool m_stopped = false;
};

} // namespace cacophony

#endif // PLAYER_WORKER_HPP
