// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef PLAYER_SERVICE_HPP
#define PLAYER_SERVICE_HPP

#include <QObject>
#include <QThread>
#include <memory>

namespace noteahead {

class Config;
class MidiService;
class PlayerWorker;
class Song;

class PlayerService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)

public:
    using ConfigS = std::shared_ptr<Config>;
    using MidiServiceS = std::shared_ptr<MidiService>;
    explicit PlayerService(MidiServiceS midiService, ConfigS config, QObject * parent = nullptr);

    ~PlayerService() override;

    Q_INVOKABLE bool play();

    Q_INVOKABLE bool isPlaying() const;

    Q_INVOKABLE void stop();

    Q_INVOKABLE void prev();

    Q_INVOKABLE void muteTrack(size_t trackIndex, bool mute);

    Q_INVOKABLE void soloTrack(size_t trackIndex, bool solo);

    void setSongPosition(size_t position);

    using SongS = std::shared_ptr<Song>;
    void setSong(SongS song);

signals:
    void isPlayingChanged();

    void songEnded();

    void songRequested();

    void tickUpdated(size_t tick);

private:
    void initializeWorker();

    void initializeWorkerWithSongData();

    void startWorker();

    void stopWorker();

    SongS m_song;

    ConfigS m_config;

    std::unique_ptr<PlayerWorker> m_playerWorker;

    QThread m_playerWorkerThread;

    size_t m_songPosition = 0;
};

} // namespace noteahead

#endif // PLAYER_SERVICE_HPP
