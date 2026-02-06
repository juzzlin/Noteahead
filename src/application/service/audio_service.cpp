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

#include <sndfile.h>

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
    m_audioWorkerThread.start(QThread::HighPriority);
}

void AudioService::startRecording(QString filePath, quint32 bufferSize)
{
    m_isRecording = true;
    emit isRecordingChanged();
    m_currentRecordingFileName = filePath;
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
    if (!m_currentRecordingFileName.isEmpty()) {
        m_latestRecordingFileName = m_currentRecordingFileName;
        m_currentRecordingFileName.clear();
        emit latestRecordingFileNameChanged();
    }
    m_isRecording = false;
    emit isRecordingChanged();
}

QVariantList AudioService::getInputDevices()
{
    const auto functionName = "getInputDevices";
    QVariantList devices;
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVariantList, devices)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
    return devices;
}

void AudioService::setInputDevice(int deviceId)
{
    const auto functionName = "setInputDevice";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Q_ARG(int, deviceId)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
}

QString AudioService::latestRecordingFileName() const
{
    return m_latestRecordingFileName;
}

bool AudioService::isRecording() const
{
    return m_isRecording;
}

QVector<double> AudioService::getWaveformData(int numPoints)
{
    if (m_latestRecordingFileName.isEmpty()) {
        return {};
    }

    SF_INFO sfInfo = {};
    SNDFILE * sndFile = sf_open(m_latestRecordingFileName.toStdString().c_str(), SFM_READ, &sfInfo);
    if (!sndFile) {
        juzzlin::L(TAG).error() << "Could not open audio file: " << m_latestRecordingFileName.toStdString();
        return {};
    }

    if (sfInfo.frames <= 0 || numPoints <= 0) {
        sf_close(sndFile);
        return {};
    }

    QVector<double> points;
    points.reserve(numPoints);

    const int channels = sfInfo.channels;
    const qint64 framesPerPoint = std::max(1ll, static_cast<long long>(sfInfo.frames) / numPoints);
    std::vector<double> buffer(framesPerPoint * channels);

    for (int i = 0; i < numPoints; ++i) {
        const auto readFrames = sf_readf_double(sndFile, buffer.data(), framesPerPoint);
        if (readFrames <= 0) break;

        double maxVal = 0.0;
        for (int j = 0; j < readFrames * channels; ++j) {
            maxVal = std::max(maxVal, std::abs(buffer[j]));
        }
        points.append(maxVal);
    }

    sf_close(sndFile);
    return points;
}

AudioService::~AudioService()
{
    juzzlin::L(TAG).info() << "Stopping worker threads";

    m_audioWorkerThread.exit();
    m_audioWorkerThread.wait();
}

} // namespace noteahead
