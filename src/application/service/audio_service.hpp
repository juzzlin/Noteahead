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

#include <QObject>
#include <QString>
#include <QThread>
#include <QVariantList>
#include <QVector>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class AudioWorker;
class SettingsService;
class JackService;

class AudioService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString latestRecordingFileName READ latestRecordingFileName NOTIFY latestRecordingFileNameChanged)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(bool isPlayingPlayback READ isPlayingPlayback NOTIFY isPlayingPlaybackChanged)
    Q_PROPERTY(double playbackPosition READ playbackPosition WRITE setPlaybackPosition NOTIFY playbackPositionChanged)
    Q_PROPERTY(quint64 latestRecordingStartTick READ latestRecordingStartTick NOTIFY latestRecordingStartTickChanged)
    Q_PROPERTY(quint64 latestRecordingEndTick READ latestRecordingEndTick NOTIFY latestRecordingEndTickChanged)

public:
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    using JackServiceS = std::shared_ptr<JackService>;
    AudioService(SettingsServiceS settingsService, JackServiceS jackService, QObject * parent = nullptr);
    ~AudioService() override;

public slots:
    void reinitialize();

public:
    Q_INVOKABLE void startRecording(QString filePath, quint32 bufferSize, quint64 startTick);
    Q_INVOKABLE void stopRecording(quint64 stopTick);

    Q_INVOKABLE void startPlayback(QString filePath, quint32 bufferSize);
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void setLatestRecordingInfo(QString filePath, quint64 startTick, quint64 endTick);
    Q_INVOKABLE bool isPlayingPlayback() const;
    Q_INVOKABLE void setPlaybackPosition(double position);
    Q_INVOKABLE double playbackPosition() const;

    Q_INVOKABLE QVariantList getInputDevices();
    Q_INVOKABLE void setInputDevice(int deviceId);
    Q_INVOKABLE QVariantList getOutputDevices();
    Q_INVOKABLE void setOutputDevice(int deviceId);

    Q_INVOKABLE QString latestRecordingFileName() const;
    Q_INVOKABLE bool isRecording() const;
    Q_INVOKABLE QVariantList getWaveformData(int numPoints);
    Q_INVOKABLE quint64 latestRecordingStartTick() const;
    Q_INVOKABLE quint64 latestRecordingEndTick() const;

    void serializeToXml(QXmlStreamWriter & writer) const;
    void deserializeFromXml(QXmlStreamReader & reader);

signals:
    void latestRecordingFileNameChanged();
    void isRecordingChanged();
    void isPlayingPlaybackChanged();
    void playbackPositionChanged();
    void latestRecordingStartTickChanged();
    void latestRecordingEndTickChanged();
    void reinitialized();

private:
    void initializeWorker();

    std::unique_ptr<AudioWorker> m_audioWorker;
    QThread m_audioWorkerThread;
    QString m_latestRecordingFileName;
    QString m_currentRecordingFileName;
    bool m_isRecording = false;
    bool m_isPlayingPlayback = false;
    double m_playbackPosition = 0.0;
    quint64 m_latestRecordingStartTick = 0;
    quint64 m_currentRecordingStartTick = 0;
    quint64 m_latestRecordingEndTick = 0;
    quint64 m_currentRecordingEndTick = 0;

    SettingsServiceS m_settingsService;
    JackServiceS m_jackService;
};

} // namespace noteahead

#endif // AUDIO_SERVICE_HPP
