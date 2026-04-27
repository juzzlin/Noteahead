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

#include "../../common/constants.hpp"
#include "../../common/waveform_generator.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/audio/implementation/jack/audio_recorder_jack.hpp"
#include "../../infra/audio/implementation/jack/audio_player_jack.hpp"
#include "../../infra/audio/implementation/librtaudio/audio_recorder_rt_audio.hpp"
#include "../../infra/audio/implementation/librtaudio/audio_player_rt_audio.hpp"
#include "audio_worker.hpp"
#include "settings_service.hpp"

#include <RtAudio.h>
#include <sndfile.h>
#include <QFile>
#include <QTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {
static const auto TAG = "AudioService";

AudioService::AudioService(SettingsServiceS settingsService, JackServiceS jackService, AudioEngineS audioEngine, QObject * parent, bool autoInitialize)
  : QObject { parent }
  , m_settingsService { std::move(settingsService) }
  , m_jackService { std::move(jackService) }
  , m_audioEngine { std::move(audioEngine) }
{
    connect(m_settingsService.get(), &SettingsService::jackSyncEnabledChanged, this, &AudioService::reinitialize);

    if (autoInitialize) {
        reinitialize();
    }
}

void AudioService::reinitialize()
{
    juzzlin::L(TAG).info() << "Reinitializing audio engine...";

    if (m_isRecording) {
        stopRecording(m_currentRecordingStartTick);
    }

    if (m_isPlayingPlayback) {
        stopPlayback();
    }

    if (m_audioWorker) {
        m_audioWorkerThread.quit();
        m_audioWorkerThread.wait();
        m_audioWorker.reset();
    }

    std::unique_ptr<AudioRecorder> audioRecorder;
    std::unique_ptr<AudioPlayer> audioPlayer;

    if (m_settingsService->jackSyncEnabled()) {
        audioRecorder = std::make_unique<AudioRecorderJack>(m_jackService);
        audioPlayer = std::make_unique<AudioPlayerJack>(m_jackService);
    } else {
        audioRecorder = std::make_unique<AudioRecorderRtAudio>(m_audioEngine, RtAudio::UNSPECIFIED);
        audioPlayer = std::make_unique<AudioPlayerRtAudio>(m_audioEngine, RtAudio::UNSPECIFIED);
    }

    m_audioWorker = std::make_unique<AudioWorker>(std::move(audioRecorder), std::move(audioPlayer), m_audioEngine);
    initializeWorker();

    emit reinitialized();
}

void AudioService::initializeWorker()
{
    connect(m_audioWorker.get(), &AudioWorker::errorOccurred, this, &AudioService::onErrorOccurred);
    connect(m_audioWorker.get(), &AudioWorker::playbackPositionChanged, this, [this](double pos) {
        if (std::abs(m_playbackPosition - pos) > 0.0001) {
            m_playbackPosition = pos;
            emit playbackPositionChanged();
        }
    });
    connect(m_audioWorker.get(), &AudioWorker::playbackFinished, this, &AudioService::stopPlayback);

    m_audioWorker->moveToThread(&m_audioWorkerThread);
    m_audioWorkerThread.start(QThread::HighPriority);

    const auto functionName = "initializeRealTimeStream";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Q_ARG(quint32, m_settingsService->audioBufferSize())); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
}

void AudioService::onErrorOccurred(QString message)
{
    if (m_isRecording) {
        m_isRecording = false;
        emit isRecordingChanged();
        m_currentRecordingFileName.clear();
    }
    if (m_isPlayingPlayback) {
        m_isPlayingPlayback = false;
        emit isPlayingPlaybackChanged();
    }
    emit errorOccurred(message);
}

void AudioService::startRecording(QString filePath, quint32 bufferSize, quint64 startTick)
{
    m_isRecording = true;
    emit isRecordingChanged();
    m_currentRecordingFileName = filePath;
    m_currentRecordingStartTick = startTick;
    const auto functionName = "startRecording";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Q_ARG(QString, filePath), Q_ARG(quint32, bufferSize)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
}

void AudioService::stopRecording(quint64 stopTick)
{
    const auto functionName = "stopRecording";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
    if (!m_currentRecordingFileName.isEmpty()) {
        m_latestRecordingFileName = m_currentRecordingFileName;
        m_currentRecordingFileName.clear();
        m_latestRecordingStartTick = m_currentRecordingStartTick;
        m_latestRecordingEndTick = stopTick;
        emit latestRecordingFileNameChanged();
        emit latestRecordingStartTickChanged();
        emit latestRecordingEndTickChanged();
    }
    m_isRecording = false;
    emit isRecordingChanged();
}

void AudioService::startPlayback(QString filePath, quint32 bufferSize)
{
    m_isPlayingPlayback = true;
    emit isPlayingPlaybackChanged();
    const auto functionName = "startPlayback";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Q_ARG(QString, filePath), Q_ARG(quint32, bufferSize)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
}

void AudioService::stopPlayback()
{
    const auto functionName = "stopPlayback";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
    m_isPlayingPlayback = false;
    emit isPlayingPlaybackChanged();
}

void AudioService::setLatestRecordingInfo(QString filePath, quint64 startTick, quint64 endTick)
{
    m_latestRecordingFileName = filePath;
    m_latestRecordingStartTick = startTick;
    m_latestRecordingEndTick = endTick;
    emit latestRecordingFileNameChanged();
    emit latestRecordingStartTickChanged();
    emit latestRecordingEndTickChanged();
}

bool AudioService::isPlayingPlayback() const
{
    return m_isPlayingPlayback;
}

void AudioService::setPlaybackPosition(double position)
{
    const auto functionName = "setPlaybackPosition";
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Q_ARG(double, position)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
    m_playbackPosition = position;
    emit playbackPositionChanged();
}

double AudioService::playbackPosition() const
{
    return m_playbackPosition;
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

QVariantList AudioService::getOutputDevices()
{
    const auto functionName = "getOutputDevices";
    QVariantList devices;
    if (const bool invoked = QMetaObject::invokeMethod(m_audioWorker.get(), functionName, Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVariantList, devices)); !invoked) {
        juzzlin::L(TAG).error() << "Invoking a method failed!: " << functionName;
    }
    return devices;
}

void AudioService::setOutputDevice(int deviceId)
{
    const auto functionName = "setOutputDevice";
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

QVariantList AudioService::getWaveformData(int numPoints)
{
    return WaveformGenerator::getWaveformData(m_latestRecordingFileName, numPoints);
}

quint64 AudioService::latestRecordingStartTick() const
{
    return m_latestRecordingStartTick;
}

quint64 AudioService::latestRecordingEndTick() const
{
    return m_latestRecordingEndTick;
}

void AudioService::serializeToXml(QXmlStreamWriter & writer) const
{
    if (!m_latestRecordingFileName.isEmpty()) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyAudioRecorder());
        writer.writeAttribute(Constants::NahdXml::xmlKeyLatestRecordingFilePath(), m_latestRecordingFileName);
        writer.writeAttribute(Constants::NahdXml::xmlKeyLatestRecordingStartTick(), QString::number(m_latestRecordingStartTick));
        writer.writeAttribute(Constants::NahdXml::xmlKeyLatestRecordingEndTick(), QString::number(m_latestRecordingEndTick));
        writer.writeEndElement();
    }
}

void AudioService::deserializeFromXml(QXmlStreamReader & reader)
{
    const auto filePath = reader.attributes().value(Constants::NahdXml::xmlKeyLatestRecordingFilePath()).toString();
    const auto startTick = reader.attributes().value(Constants::NahdXml::xmlKeyLatestRecordingStartTick()).toULongLong();
    const auto endTick = reader.attributes().value(Constants::NahdXml::xmlKeyLatestRecordingEndTick()).toULongLong();
    if (QFile::exists(filePath)) {
        setLatestRecordingInfo(filePath, startTick, endTick);
    }
}

AudioService::~AudioService()
{
    juzzlin::L(TAG).info() << "Stopping worker threads";

    m_audioWorkerThread.exit();
    m_audioWorkerThread.wait();
}

} // namespace noteahead
