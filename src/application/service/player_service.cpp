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

#include "player_service.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/song.hpp"
#include "jack_service.hpp"
#include "player_worker.hpp"
#include "settings_service.hpp"

#include <QThread>

namespace noteahead {

static const auto TAG = "PlayerService";

PlayerService::PlayerService(
  MidiServiceS midiService,
  MixerServiceS mixerService,
  AutomationServiceS automationService,
  SettingsServiceS settingsService,
  SideChainServiceS sideChainService,
  JackServiceS jackService,
  QObject * parent)
  : QObject { parent }
  , m_automationService { automationService }
  , m_settingsService { settingsService }
  , m_sideChainService { sideChainService }
  , m_jackService { jackService }
  , m_playerWorker { std::make_unique<PlayerWorker>(midiService, mixerService, jackService) }
{
    initializeWorker();
    connect(m_jackService.get(), &JackService::bpmChanged, this, [this]() {
        if (m_settingsService->jackBpmSyncEnabled()) {
            emit beatsPerMinuteChanged();
        }
    });
}

void PlayerService::setSongPosition(quint64 position)
{
    m_songPosition = position;
}

void PlayerService::setSong(SongS song)
{
    m_song = song;
}

void PlayerService::initializeWorker()
{
    connect(m_playerWorker.get(), &PlayerWorker::tickUpdated, this, &PlayerService::tickUpdated, Qt::QueuedConnection);
    connect(m_playerWorker.get(), &PlayerWorker::isPlayingChanged, this, &PlayerService::isPlayingChanged, Qt::QueuedConnection);
    connect(m_playerWorker.get(), &PlayerWorker::songEnded, this, &PlayerService::songEnded, Qt::QueuedConnection);
    m_playerWorker->moveToThread(&m_playerWorkerThread);
    m_playerWorkerThread.start(QThread::HighestPriority);
}

void PlayerService::initializeWorkerWithSongData()
{
    const PlayerWorker::Timing timing { m_song->beatsPerMinute(), m_song->linesPerBeat(), m_song->ticksPerLine() };
    m_song->setAutoNoteOffOffset(std::chrono::milliseconds { m_settingsService->autoNoteOffOffset() });
    m_playerWorker->setJackBpmSyncEnabled(m_settingsService->jackBpmSyncEnabled());
    if (m_playerWorker->isLooping()) {
        m_playerWorker->initialize(m_song->renderToEvents(m_automationService, m_sideChainService, m_songPosition, m_songPosition + 1), timing);
    } else {
        m_playerWorker->initialize(m_song->renderToEvents(m_automationService, m_sideChainService, m_songPosition), timing);
    }
}

void PlayerService::startWorker()
{
    juzzlin::L(TAG).debug() << "Starting playback";
    QMetaObject::invokeMethod(m_playerWorker.get(), "play", Qt::QueuedConnection);
}

void PlayerService::stopWorker()
{
    if (m_playerWorker) {
        m_playerWorker->stop();
    }
}

bool PlayerService::play()
{
    if (isPlaying()) {
        return true;
    }

    juzzlin::L(TAG).debug() << "Song for playback requested";

    emit songRequested();

    if (m_song) {
        initializeWorkerWithSongData();
        startWorker();
        return true;
    }

    return false;
}

bool PlayerService::isPlaying() const
{
    return m_playerWorker->isPlaying();
}

void PlayerService::stop()
{
    juzzlin::L(TAG).debug() << "Stop requested";
    stopWorker();
    m_song.reset();
}

void PlayerService::prev()
{
    juzzlin::L(TAG).debug() << "Prev requested";
}

bool PlayerService::isLooping() const
{
    return m_playerWorker->isLooping();
}

void PlayerService::setIsLooping(bool isLooping)
{
    m_playerWorker->setIsLooping(isLooping);
    emit isLoopingChanged();
}

double PlayerService::beatsPerMinute() const
{
    if (m_settingsService->jackBpmSyncEnabled()) {
        return m_jackService->bpm();
    }
    return m_song ? m_song->beatsPerMinute() : 120.0;
}

PlayerService::~PlayerService()
{
    stop();

    juzzlin::L(TAG).debug() << "Stopping playback thread";

    m_playerWorkerThread.exit();
    m_playerWorkerThread.wait();

    juzzlin::L(TAG).debug() << "Deleted";
}

} // namespace noteahead
