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

#include "audio_service.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "audio_worker.hpp"

namespace noteahead {

static const auto TAG = "AudioService";

AudioService::AudioService(QObject * parent)
  : QObject { parent }
  , m_audioWorker { std::make_unique<AudioWorker>() }
{
    initializeWorker();
}

void AudioService::initializeWorker()
{
    m_audioWorker->moveToThread(&m_audioWorkerThread);
    m_audioWorkerThread.start(QThread::HighestPriority);
}

void AudioService::startRecording(QString filePath, quint32 bufferSize)
{
    const auto functionName = "startRecording";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Q_ARG(QString, filePath), Q_ARG(quint32, bufferSize)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
}

void AudioService::stopRecording()
{
    const auto functionName = "stopRecording";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
}

AudioService::~AudioService()
{
    juzzlin::L(TAG).info() << "Stopping worker threads";

    m_audioWorkerThread.exit();
    m_audioWorkerThread.wait();
}

} // namespace noteahead
