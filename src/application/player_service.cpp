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

#include "player_service.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/song.hpp"
#include "player_worker.hpp"

#include <QThread>

namespace cacophony {

static const auto TAG = "PlayerService";

PlayerService::PlayerService(QObject * parent)
  : QObject { parent }
  , m_playerWorker { std::make_unique<PlayerWorker>() }
{
    initializeWorker();
}

void PlayerService::setSong(SongS song)
{
    m_song = song;
}

void PlayerService::initializeWorker()
{
    connect(m_playerWorker.get(), &PlayerWorker::songEnded, this, [this] {
        stop();
    });
    connect(m_playerWorker.get(), &PlayerWorker::tickUpdated, this, &PlayerService::tickUpdated, Qt::QueuedConnection);
    m_playerWorker->moveToThread(&m_playerWorkerThread);
    m_playerWorkerThread.start(QThread::HighPriority);
}

void PlayerService::initializeWorkerWithSongData()
{
    const PlayerWorker::Timing timing { m_song->beatsPerMinute(), m_song->linesPerBeat(), m_song->ticksPerLine() };
    m_playerWorker->initialize(m_song->renderToEvents(), timing);
}

void PlayerService::startPlayback()
{
    juzzlin::L(TAG).debug() << "Starting playback";
    QMetaObject::invokeMethod(m_playerWorker.get(), "play", Qt::QueuedConnection);
    emit isPlayingChanged();
}

bool PlayerService::requestPlay()
{
    juzzlin::L(TAG).debug() << "Song for playback requested";

    emit songRequested();

    if (m_song) {
        initializeWorkerWithSongData();
        startPlayback();
        return true;
    }

    return false;
}

bool PlayerService::isPlaying() const
{
    return !m_playerWorker->isStopped();
}

void PlayerService::stop()
{
    if (m_playerWorker) {
        m_playerWorker->stop();
    }

    emit isPlayingChanged();
}

void PlayerService::requestStop()
{
    juzzlin::L(TAG).debug() << "Stop requested";
    stop();
    emit isPlayingChanged();
    m_song.reset();
}

void PlayerService::requestPrev()
{
    juzzlin::L(TAG).debug() << "Prev requested";
}

PlayerService::~PlayerService()
{
    stop();

    juzzlin::L(TAG).debug() << "Stopping playback thread";

    m_playerWorkerThread.exit();
    m_playerWorkerThread.wait();

    juzzlin::L(TAG).debug() << "Deleted";
}

} // namespace cacophony
