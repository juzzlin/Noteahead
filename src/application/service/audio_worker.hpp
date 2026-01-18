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

#include <memory>

namespace noteahead {

class AudioRecorder;

class AudioWorker : public QObject
{
    Q_OBJECT

public:
    AudioWorker(QObject * parent = nullptr);
    ~AudioWorker() override;

    Q_INVOKABLE void startRecording(QString filePath, quint32 bufferSize);
    Q_INVOKABLE void stopRecording();

    Q_INVOKABLE QVariantList getInputDevices() const;
    Q_INVOKABLE void setInputDevice(int deviceId);

private:
    std::unique_ptr<AudioRecorder> m_audioRecorder;
};

} // namespace noteahead

#endif // AUDIO_WORKER_HPP
