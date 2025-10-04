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

#include "../../infra/audio/implementation/librtaudio/audio_recorder_rt_audio.hpp"

namespace noteahead {

AudioWorker::AudioWorker(QObject * parent)
  : QObject { parent }
  , m_audioRecorder { std::make_unique<AudioRecorderRtAudio>() }
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

AudioWorker::~AudioWorker() = default;

} // namespace noteahead
