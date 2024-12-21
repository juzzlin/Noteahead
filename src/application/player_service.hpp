// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef PLAYER_SERVICE_HPP
#define PLAYER_SERVICE_HPP

#include <QObject>
#include <QThread>
#include <memory>

namespace cacophony {

class PlayerWorker;
class Song;

class PlayerService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)

public:
    explicit PlayerService(QObject * parent = nullptr);

    ~PlayerService();

    Q_INVOKABLE bool requestPlay();

    Q_INVOKABLE bool isPlaying() const;

    Q_INVOKABLE void requestStop();

    Q_INVOKABLE void requestPrev();

    using SongS = std::shared_ptr<Song>;
    void setSong(SongS song);

signals:
    void isPlayingChanged();

    void songEnded();

    void songRequested();

    void tickUpdated(uint32_t tick);

private:
    void initializeWorker();

    void startPlayback();

    void stop();

    SongS m_song;

    std::unique_ptr<PlayerWorker> m_playerWorker;

    QThread m_playerWorkerThread;
};

} // namespace cacophony

#endif // PLAYER_SERVICE_HPP
