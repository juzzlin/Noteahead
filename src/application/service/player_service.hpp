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

class AutomationService;
class JackService;
class MidiService;
class MixerService;
class PlayerWorker;
class SettingsService;
class SideChainService;
class Song;

class PlayerService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isLooping READ isLooping WRITE setIsLooping NOTIFY isLoopingChanged)
    Q_PROPERTY(double beatsPerMinute READ beatsPerMinute NOTIFY beatsPerMinuteChanged)

public:
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    using MidiServiceS = std::shared_ptr<MidiService>;
    using MixerServiceS = std::shared_ptr<MixerService>;
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using SideChainServiceS = std::shared_ptr<SideChainService>;
    using JackServiceS = std::shared_ptr<JackService>;
    explicit PlayerService(
      MidiServiceS midiService,
      MixerServiceS mixerService,
      AutomationServiceS automationService,
      SettingsServiceS settingsService,
      SideChainServiceS sideChainService,
      JackServiceS jackService,
      QObject * parent = nullptr);

    ~PlayerService() override;

    virtual Q_INVOKABLE bool play();
    virtual Q_INVOKABLE bool isPlaying() const;
    virtual Q_INVOKABLE void stop();
    virtual Q_INVOKABLE void prev();

    Q_INVOKABLE bool isLooping() const;
    Q_INVOKABLE void setIsLooping(bool isLooping);

    Q_INVOKABLE double beatsPerMinute() const;

    quint64 tick() const;

    void setSongPosition(quint64 position);

    using SongS = std::shared_ptr<Song>;
    void setSong(SongS song);

signals:
    void isPlayingChanged();
    void isLoopingChanged();

    void songEnded();

    void songRequested();

    void tickUpdated(quint64 tick);
    void beatsPerMinuteChanged();

private:
    void initializeWorker();
    void initializeWorkerWithSongData();

    void startWorker();
    void stopWorker();

    SongS m_song;

    MidiServiceS m_midiService;
    AutomationServiceS m_automationService;
    SettingsServiceS m_settingsService;
    SideChainServiceS m_sideChainService;
    JackServiceS m_jackService;

    std::unique_ptr<PlayerWorker> m_playerWorker;

    QThread m_playerWorkerThread;

    quint64 m_songPosition = 0;
    quint64 m_tick = 0;
};

} // namespace noteahead

#endif // PLAYER_SERVICE_HPP
