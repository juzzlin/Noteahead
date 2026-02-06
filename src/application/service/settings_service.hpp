// This file is part of Noteahead.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef SETTINGS_SERVICE_H
#define SETTINGS_SERVICE_H

#include <QObject>
#include <QSize>

namespace noteahead {

class SettingsService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString controllerPort READ controllerPort WRITE setControllerPort NOTIFY controllerPortChanged)
    Q_PROPERTY(bool uiUpdatesDisabledDuringPlayback READ uiUpdatesDisabledDuringPlayback WRITE setUiUpdatesDisabledDuringPlayback NOTIFY uiUpdatesDisabledDuringPlaybackChanged)
    Q_PROPERTY(int visibleLines READ visibleLines WRITE setVisibleLines NOTIFY visibleLinesChanged)
    Q_PROPERTY(int trackHeaderFontSize READ trackHeaderFontSize WRITE setTrackHeaderFontSize NOTIFY trackHeaderFontSizeChanged)
    Q_PROPERTY(bool recordingEnabled READ recordingEnabled WRITE setRecordingEnabled NOTIFY recordingEnabledChanged)

public:
    SettingsService();
    ~SettingsService() override;

    virtual Q_INVOKABLE int autoNoteOffOffset() const;
    virtual Q_INVOKABLE void setAutoNoteOffOffset(int autoNoteOffOffset);

    virtual Q_INVOKABLE QString controllerPort() const;
    virtual Q_INVOKABLE void setControllerPort(QString controllerPort);

    virtual Q_INVOKABLE bool uiUpdatesDisabledDuringPlayback() const;
    virtual Q_INVOKABLE void setUiUpdatesDisabledDuringPlayback(bool disabled);

    virtual Q_INVOKABLE QSize windowSize(QSize defaultSize) const;
    virtual Q_INVOKABLE void setWindowSize(QSize size);

    virtual Q_INVOKABLE int step(int defaultStep) const;
    virtual Q_INVOKABLE void setStep(int step);

    virtual Q_INVOKABLE int velocity(int defaultVelocity) const;
    virtual Q_INVOKABLE void setVelocity(int velocity);

    virtual Q_INVOKABLE int visibleLines() const;
    virtual Q_INVOKABLE void setVisibleLines(int visibleLines);

    virtual Q_INVOKABLE int trackHeaderFontSize() const;
    virtual Q_INVOKABLE void setTrackHeaderFontSize(int trackHeaderFontSize);

    virtual Q_INVOKABLE bool recordingEnabled() const;
    virtual Q_INVOKABLE void setRecordingEnabled(bool enabled);

    virtual Q_INVOKABLE int audioBufferSize() const;
    virtual Q_INVOKABLE void setAudioBufferSize(int bufferSize);

    virtual Q_INVOKABLE int audioInputDeviceId() const;
    virtual Q_INVOKABLE void setAudioInputDeviceId(int deviceId);

signals:
    void controllerPortChanged();
    void uiUpdatesDisabledDuringPlaybackChanged();

    void visibleLinesChanged();
    void trackHeaderFontSizeChanged();
    void recordingEnabledChanged();

private:
    QString m_controllerPort;
    bool m_uiUpdatesDisabledDuringPlayback;

    int m_visibleLines;
    int m_trackHeaderFontSize;
};

} // namespace noteahead

#endif // SETTINGS_SERVICE_H
