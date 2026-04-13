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

#ifndef AUDIO_WORKER_HPP
#define AUDIO_WORKER_HPP

#include <QObject>
#include <QVariantList>

#include <RtAudio.h>
#include <memory>

namespace noteahead {

class AudioRecorder;
class AudioPlayer;

class AudioWorker : public QObject
{
    Q_OBJECT

public:
    AudioWorker(std::unique_ptr<AudioRecorder> audioRecorder, std::unique_ptr<AudioPlayer> audioPlayer, QObject * parent = nullptr);
    ~AudioWorker() override;

    Q_INVOKABLE void startRecording(QString filePath, quint32 bufferSize);
    Q_INVOKABLE void stopRecording();

    Q_INVOKABLE void startPlayback(QString filePath, quint32 bufferSize);
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE bool isPlaybackFinished() const;
    Q_INVOKABLE void setPlaybackPosition(double position);
    Q_INVOKABLE double playbackPosition() const;

    Q_INVOKABLE QVariantList getInputDevices() const;
    Q_INVOKABLE void setInputDevice(int deviceId);
    Q_INVOKABLE QVariantList getOutputDevices() const;
    Q_INVOKABLE void setOutputDevice(int deviceId);

private:
    std::unique_ptr<AudioRecorder> m_audioRecorder;
    std::unique_ptr<AudioPlayer> m_audioPlayer;
};

} // namespace noteahead

#endif // AUDIO_WORKER_HPP
