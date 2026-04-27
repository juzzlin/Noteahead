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

#include "audio_worker.hpp"
#include "../../infra/audio/audio_player.hpp"
#include "../../infra/audio/audio_recorder.hpp"

#include <QVariantMap>

namespace noteahead {

AudioWorker::AudioWorker(std::unique_ptr<AudioRecorder> audioRecorder, std::unique_ptr<AudioPlayer> audioPlayer, AudioEngineS audioEngine, QObject * parent)
  : QObject { parent }
  , m_audioRecorder { std::move(audioRecorder) }
  , m_audioPlayer { std::move(audioPlayer) }
  , m_audioEngine { std::move(audioEngine) }
  , m_statusTimer { new QTimer(this) }
{
    connect(m_statusTimer, &QTimer::timeout, this, &AudioWorker::onStatusTimerTimeout);
}

void AudioWorker::initializeRealTimeStream(quint32 bufferSize)
{
    m_bufferSize = bufferSize;
    if (m_audioEngine) {
        try {
            // Start empty playback if not already running to ensure audio stream is active for real-time sound
            m_audioPlayer->start("", bufferSize);
        } catch (...) {
            // It might fail if no device is available, but we don't want to emit error here
        }
    }
}

void AudioWorker::startRecording(QString filePath, quint32 bufferSize)
{
    try {
        m_audioRecorder->start(filePath.toStdString(), bufferSize);
    } catch (const std::exception & e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AudioWorker::stopRecording()
{
    m_audioRecorder->stop();
}

void AudioWorker::startPlayback(QString filePath, quint32 bufferSize)
{
    try {
        m_audioPlayer->start(filePath.toStdString(), bufferSize);
        m_statusTimer->start(50);
    } catch (const std::exception & e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void AudioWorker::stopPlayback()
{
    m_audioPlayer->stop();
    m_statusTimer->stop();

    if (m_audioEngine && m_bufferSize > 0) {
        initializeRealTimeStream(m_bufferSize);
    }
}

bool AudioWorker::isPlaybackFinished() const
{
    return m_audioPlayer->isFinished();
}

void AudioWorker::setPlaybackPosition(double position)
{
    m_audioPlayer->setPosition(position);
}

double AudioWorker::playbackPosition() const
{
    return m_audioPlayer->position();
}

void AudioWorker::onStatusTimerTimeout()
{
    const double pos = m_audioPlayer->position();
    const bool finished = m_audioPlayer->isFinished();

    emit playbackPositionChanged(pos);

    if (finished) {
        m_statusTimer->stop();
        emit playbackFinished();
    }
}

QVariantList AudioWorker::getInputDevices() const
{
    QVariantList list;
    for (const auto & device : m_audioRecorder->getInputDevices()) {
        QVariantMap map;
        map.insert("id", device.id);
        map.insert("name", QString::fromStdString(device.name));
        list.append(map);
    }
    return list;
}

void AudioWorker::setInputDevice(int deviceId)
{
    m_audioRecorder->setInputDevice(static_cast<uint32_t>(deviceId));
}

QVariantList AudioWorker::getOutputDevices() const
{
    QVariantList list;
    for (const auto & device : m_audioPlayer->getOutputDevices()) {
        QVariantMap map;
        map.insert("id", device.id);
        map.insert("name", QString::fromStdString(device.name));
        list.append(map);
    }
    return list;
}

void AudioWorker::setOutputDevice(int deviceId)
{
    m_audioPlayer->setOutputDevice(static_cast<uint32_t>(deviceId));
}

AudioWorker::~AudioWorker() = default;

} // namespace noteahead
