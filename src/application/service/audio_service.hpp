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

#ifndef AUDIO_SERVICE_HPP
#define AUDIO_SERVICE_HPP

#include <memory>

#include <QThread>

namespace noteahead {

class AudioWorker;

class AudioService : public QObject
{
    Q_OBJECT

public:
    AudioService(QObject * parent = nullptr);
    ~AudioService() override;

    void startRecording(QString filePath);
    void stopRecording();

private:
    void initializeWorker();

    std::unique_ptr<AudioWorker> m_audioWorker;
    QThread m_audioWorkerThread;
};

} // namespace noteahead

#endif // AUDIO_SERVICE_HPP
