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

AudioWorker::AudioWorker(std::unique_ptr<AudioRecorder> audioRecorder, std::unique_ptr<AudioPlayer> audioPlayer, QObject * parent)
  : QObject { parent }
  , m_audioRecorder { std::move(audioRecorder) }
  , m_audioPlayer { std::move(audioPlayer) }
{
}

void AudioWorker::startRecording(QString filePath, quint32 bufferSize)
{
    m_audioRecorder->start(filePath.toStdString(), bufferSize);
}

void AudioWorker::stopRecording()
{
    m_audioRecorder->stop();
}

void AudioWorker::startPlayback(QString filePath, quint32 bufferSize)
{
    m_audioPlayer->start(filePath.toStdString(), bufferSize);
}

void AudioWorker::stopPlayback()
{
    m_audioPlayer->stop();
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

QVariantList AudioWorker::getInputDevices() const
{
    QVariantList list;
    for (const auto & device : m_audioRecorder->getInputDevices()) {
        QVariantMap map;
        map["id"] = static_cast<int>(device.id);
        map["name"] = QString::fromStdString(device.name);
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
        map["id"] = static_cast<int>(device.id);
        map["name"] = QString::fromStdString(device.name);
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
