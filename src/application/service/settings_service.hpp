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

#include <optional>

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
    Q_PROPERTY(bool autoTrackCount READ autoTrackCount WRITE setAutoTrackCount NOTIFY autoTrackCountChanged)
    Q_PROPERTY(bool recordingEnabled READ recordingEnabled WRITE setRecordingEnabled NOTIFY recordingEnabledChanged)
    Q_PROPERTY(int audioBackend READ audioBackend WRITE setAudioBackend NOTIFY audioBackendChanged)
    Q_PROPERTY(bool multiThreadedPlaybackEnabled READ multiThreadedPlaybackEnabled WRITE setMultiThreadedPlaybackEnabled NOTIFY multiThreadedPlaybackEnabledChanged)
    Q_PROPERTY(bool jackSyncEnabled READ jackSyncEnabled WRITE setJackSyncEnabled NOTIFY jackSyncEnabledChanged)
    Q_PROPERTY(bool jackBpmSyncEnabled READ jackBpmSyncEnabled WRITE setJackBpmSyncEnabled NOTIFY jackBpmSyncEnabledChanged)
    Q_PROPERTY(bool midiSyncEnabled READ midiSyncEnabled WRITE setMidiSyncEnabled NOTIFY midiSyncEnabledChanged)
    Q_PROPERTY(bool waveViewEnabled READ waveViewEnabled WRITE setWaveViewEnabled NOTIFY waveViewEnabledChanged)
    Q_PROPERTY(bool patternPeekEnabled READ patternPeekEnabled WRITE setPatternPeekEnabled NOTIFY patternPeekEnabledChanged)
    Q_PROPERTY(bool tipsEnabled READ tipsEnabled WRITE setTipsEnabled NOTIFY tipsEnabledChanged)
    Q_PROPERTY(bool midiExportForceDrumChannel10 READ midiExportForceDrumChannel10 WRITE setMidiExportForceDrumChannel10 NOTIFY midiExportForceDrumChannel10Changed)
    Q_PROPERTY(bool midiExportAutoAssignChannels READ midiExportAutoAssignChannels WRITE setMidiExportAutoAssignChannels NOTIFY midiExportAutoAssignChannelsChanged)
    Q_PROPERTY(int playbackOversampleFactor READ playbackOversampleFactor WRITE setPlaybackOversampleFactor NOTIFY playbackOversampleFactorChanged)
    //! Level the Device Rack's meter marker sits at, in dBFS.
    Q_PROPERTY(int gainStagingTargetDb READ gainStagingTargetDb WRITE setGainStagingTargetDb NOTIFY gainStagingTargetDbChanged)
    //! How automations are drawn over the tracker lines. See Constants::AutomationDisplayMode.
    Q_PROPERTY(int automationDisplayMode READ automationDisplayMode WRITE setAutomationDisplayMode NOTIFY automationDisplayModeChanged)
    //! Width of a drawn automation curve, in tenths of a pixel.
    Q_PROPERTY(int automationCurveThicknessTenths READ automationCurveThicknessTenths WRITE setAutomationCurveThicknessTenths NOTIFY automationCurveThicknessTenthsChanged)
    //! Language the user picked, as a Qt locale name. Empty means the system's UI languages decide.
    Q_PROPERTY(QString userLanguage READ userLanguage WRITE setUserLanguage NOTIFY userLanguageChanged)

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

    virtual Q_INVOKABLE int step() const;
    virtual Q_INVOKABLE void setStep(int step);

    virtual Q_INVOKABLE int velocity() const;
    virtual Q_INVOKABLE void setVelocity(int velocity);

    virtual Q_INVOKABLE int visibleLines() const;
    virtual Q_INVOKABLE void setVisibleLines(int visibleLines);

    virtual Q_INVOKABLE int trackHeaderFontSize() const;
    virtual Q_INVOKABLE void setTrackHeaderFontSize(int trackHeaderFontSize);

    virtual Q_INVOKABLE bool autoTrackCount() const;
    virtual Q_INVOKABLE void setAutoTrackCount(bool autoTrackCount);

    virtual Q_INVOKABLE bool recordingEnabled() const;
    virtual Q_INVOKABLE void setRecordingEnabled(bool enabled);

    virtual Q_INVOKABLE int audioBackend() const;
    virtual Q_INVOKABLE void setAudioBackend(int audioBackend);

    virtual Q_INVOKABLE bool multiThreadedPlaybackEnabled() const;
    virtual Q_INVOKABLE void setMultiThreadedPlaybackEnabled(bool enabled);

    virtual Q_INVOKABLE bool jackSyncEnabled() const;
    virtual Q_INVOKABLE void setJackSyncEnabled(bool enabled);

    virtual Q_INVOKABLE bool jackBpmSyncEnabled() const;
    virtual Q_INVOKABLE void setJackBpmSyncEnabled(bool enabled);

    virtual Q_INVOKABLE bool midiSyncEnabled() const;
    virtual Q_INVOKABLE void setMidiSyncEnabled(bool enabled);

    virtual Q_INVOKABLE bool waveViewEnabled() const;
    virtual Q_INVOKABLE void setWaveViewEnabled(bool enabled);

    virtual Q_INVOKABLE bool patternPeekEnabled() const;
    virtual Q_INVOKABLE void setPatternPeekEnabled(bool enabled);

    virtual Q_INVOKABLE bool tipsEnabled() const;
    virtual Q_INVOKABLE void setTipsEnabled(bool enabled);

    virtual Q_INVOKABLE bool midiExportForceDrumChannel10() const;
    virtual Q_INVOKABLE void setMidiExportForceDrumChannel10(bool enabled);

    virtual Q_INVOKABLE bool midiExportAutoAssignChannels() const;
    virtual Q_INVOKABLE void setMidiExportAutoAssignChannels(bool enabled);

    virtual Q_INVOKABLE int audioBufferSize() const;
    virtual Q_INVOKABLE void setAudioBufferSize(int bufferSize);

    virtual Q_INVOKABLE int audioInputDeviceId() const;
    virtual Q_INVOKABLE void setAudioInputDeviceId(int deviceId);

    virtual Q_INVOKABLE int audioOutputDeviceId() const;
    virtual Q_INVOKABLE void setAudioOutputDeviceId(int deviceId);

    virtual Q_INVOKABLE int playbackOversampleFactor() const;
    virtual Q_INVOKABLE void setPlaybackOversampleFactor(int factor);

    virtual Q_INVOKABLE int gainStagingTargetDb() const;
    virtual Q_INVOKABLE void setGainStagingTargetDb(int targetDb);

    virtual Q_INVOKABLE int automationDisplayMode() const;
    virtual Q_INVOKABLE void setAutomationDisplayMode(int mode);

    virtual Q_INVOKABLE int automationCurveThicknessTenths() const;
    virtual Q_INVOKABLE void setAutomationCurveThicknessTenths(int tenths);

    virtual Q_INVOKABLE QString userLanguage() const;
    virtual Q_INVOKABLE void setUserLanguage(QString userLanguage);

signals:
    void controllerPortChanged();
    void uiUpdatesDisabledDuringPlaybackChanged();

    void visibleLinesChanged();
    void trackHeaderFontSizeChanged();
    void autoTrackCountChanged();
    void recordingEnabledChanged();
    void audioBackendChanged();
    void multiThreadedPlaybackEnabledChanged();
    void jackSyncEnabledChanged();
    void jackBpmSyncEnabledChanged();
    void midiSyncEnabledChanged();
    void waveViewEnabledChanged();
    void patternPeekEnabledChanged();
    void tipsEnabledChanged();
    void midiExportForceDrumChannel10Changed();
    void midiExportAutoAssignChannelsChanged();
    void playbackOversampleFactorChanged();
    void gainStagingTargetDbChanged();
    void automationDisplayModeChanged();
    void automationCurveThicknessTenthsChanged();
    void userLanguageChanged();

private:
    int m_autoNoteOffOffset;
    QString m_controllerPort;
    bool m_uiUpdatesDisabledDuringPlayback;

    // Unset until the user has resized the window; the caller-provided default applies until then
    std::optional<QSize> m_windowSize;

    int m_step;
    int m_velocity;
    int m_visibleLines;
    int m_trackHeaderFontSize;
    bool m_autoTrackCount;

    bool m_recordingEnabled;
    int m_audioBackend;
    int m_audioBufferSize;
    int m_audioInputDeviceId;
    int m_audioOutputDeviceId;

    bool m_multiThreadedPlaybackEnabled;
    bool m_jackSyncEnabled;
    bool m_jackBpmSyncEnabled;
    bool m_midiSyncEnabled;
    bool m_waveViewEnabled;
    bool m_patternPeekEnabled;
    bool m_tipsEnabled;

    bool m_midiExportForceDrumChannel10;
    bool m_midiExportAutoAssignChannels;

    int m_playbackOversampleFactor;
    int m_gainStagingTargetDb;
    int m_automationDisplayMode;
    int m_automationCurveThicknessTenths;

    QString m_userLanguage;
};

} // namespace noteahead

#endif // SETTINGS_SERVICE_H
