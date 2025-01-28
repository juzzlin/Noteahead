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

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/song.hpp"
#include "player_worker.hpp"

#include <QThread>

namespace noteahead {

static const auto TAG = "PlayerService";

PlayerService::PlayerService(MidiServiceS midiService, QObject * parent)
  : QObject { parent }
  , m_playerWorker { std::make_unique<PlayerWorker>(midiService) }
{
    initializeWorker();
}

void PlayerService::setPlayOrderSongPosition(size_t position)
{
    m_playOrderSongPosition = position;
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
    connect(m_playerWorker.get(), &PlayerWorker::isPlayingChanged, this, &PlayerService::isPlayingChanged, Qt::QueuedConnection);
    m_playerWorker->moveToThread(&m_playerWorkerThread);
    m_playerWorkerThread.start(QThread::HighPriority);
}

void PlayerService::initializeWorkerWithSongData()
{
    const PlayerWorker::Timing timing { m_song->beatsPerMinute(), m_song->linesPerBeat(), m_song->ticksPerLine() };
    m_playerWorker->initialize(m_song->renderToEvents(m_playOrderSongPosition), timing);
}

void PlayerService::startPlayback()
{
    juzzlin::L(TAG).debug() << "Starting playback";
    QMetaObject::invokeMethod(m_playerWorker.get(), "play", Qt::QueuedConnection);
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
    return m_playerWorker->isPlaying();
}

void PlayerService::stop()
{
    if (m_playerWorker) {
        m_playerWorker->stop();
    }
}

void PlayerService::requestStop()
{
    juzzlin::L(TAG).debug() << "Stop requested";
    stop();
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

} // namespace noteahead
