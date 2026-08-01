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

#include "settings_service.hpp"
#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/settings.hpp"

#include <QGuiApplication>
#include <QScreen>

namespace noteahead {

static const auto TAG = "SettingsService";

SettingsService::SettingsService()
  : m_autoNoteOffOffset { Settings::autoNoteOffOffset(static_cast<int>(Constants::defaultAutoNoteOffOffset().count())) }
  , m_controllerPort { Settings::controllerPort("") }
  , m_uiUpdatesDisabledDuringPlayback { Settings::uiUpdatesDisabledDuringPlayback() }
  , m_step { Settings::step(1) }
  , m_velocity { Settings::velocity(100) }
  , m_visibleLines { Settings::visibleLines(32) }
  , m_trackHeaderFontSize { Settings::trackHeaderFontSize(20) }
  , m_recordingEnabled { Settings::recordingEnabled() }
  , m_audioBackend { static_cast<int>(Settings::audioBackend()) }
  , m_audioBufferSize { Settings::audioBufferSize() }
  , m_audioInputDeviceId { Settings::audioInputDeviceId() }
  , m_audioOutputDeviceId { Settings::audioOutputDeviceId() }
  , m_multiThreadedPlaybackEnabled { Settings::multiThreadedPlaybackEnabled() }
  , m_jackSyncEnabled { Settings::jackSyncEnabled() }
  , m_jackBpmSyncEnabled { Settings::jackBpmSyncEnabled() }
  , m_midiSyncEnabled { Settings::midiSyncEnabled() }
  , m_waveViewEnabled { Settings::waveViewEnabled() }
  , m_patternPeekEnabled { Settings::patternPeekEnabled() }
  , m_midiExportForceDrumChannel10 { Settings::midiExportForceDrumChannel10() }
  , m_midiExportAutoAssignChannels { Settings::midiExportAutoAssignChannels() }
  , m_playbackOversampleFactor { Settings::playbackOversampleFactor() }
  , m_gainStagingTargetDb { Settings::gainStagingTargetDb(Constants::defaultGainStagingTargetDb()) }
  , m_automationDisplayMode { Settings::automationDisplayMode(Constants::defaultAutomationDisplayMode()) }
{
    // An invalid size means nothing has been stored yet, in which case the caller's default wins
    if (const auto storedWindowSize = Settings::windowSize(QSize {}); storedWindowSize.isValid()) {
        m_windowSize = storedWindowSize;
    }
}

int SettingsService::autoNoteOffOffset() const
{
    return m_autoNoteOffOffset;
}

void SettingsService::setAutoNoteOffOffset(int autoNoteOffOffset)
{
    if (m_autoNoteOffOffset != autoNoteOffOffset) {
        m_autoNoteOffOffset = autoNoteOffOffset;
        Settings::setAutoNoteOffOffset(autoNoteOffOffset);
    }
}

QString SettingsService::controllerPort() const
{
    return m_controllerPort;
}

void SettingsService::setControllerPort(QString controllerPort)
{
    if (m_controllerPort != controllerPort) {
        juzzlin::L(TAG).info() << "Setting controller port: " << controllerPort.toStdString();
        m_controllerPort = controllerPort;
        Settings::setControllerPort(controllerPort);
        emit controllerPortChanged();
    }
}

bool SettingsService::uiUpdatesDisabledDuringPlayback() const
{
    return m_uiUpdatesDisabledDuringPlayback;
}

void SettingsService::setUiUpdatesDisabledDuringPlayback(bool disabled)
{
    if (m_uiUpdatesDisabledDuringPlayback != disabled) {
        m_uiUpdatesDisabledDuringPlayback = disabled;
        Settings::setUiUpdatesDisabledDuringPlayback(disabled);
        emit uiUpdatesDisabledDuringPlaybackChanged();
    }
}

QSize SettingsService::windowSize(QSize defaultSize) const
{
    return m_windowSize.value_or(defaultSize);
}

void SettingsService::setWindowSize(QSize size)
{
    if (m_windowSize != size) {
        m_windowSize = size;
        Settings::setWindowSize(size);
    }
}

int SettingsService::step() const
{
    return m_step;
}

void SettingsService::setStep(int step)
{
    if (m_step != step) {
        m_step = step;
        Settings::setStep(step);
    }
}

int SettingsService::velocity() const
{
    return m_velocity;
}

void SettingsService::setVelocity(int velocity)
{
    if (m_velocity != velocity) {
        m_velocity = velocity;
        Settings::setVelocity(velocity);
    }
}

int SettingsService::visibleLines() const
{
    return m_visibleLines;
}

void SettingsService::setVisibleLines(int visibleLines)
{
    if (m_visibleLines != visibleLines) {
        m_visibleLines = visibleLines;
        Settings::setVisibleLines(visibleLines);
        emit visibleLinesChanged();
    }
}

int SettingsService::trackHeaderFontSize() const
{
    return m_trackHeaderFontSize;
}

void SettingsService::setTrackHeaderFontSize(int trackHeaderFontSize)
{
    if (m_trackHeaderFontSize != trackHeaderFontSize) {
        m_trackHeaderFontSize = trackHeaderFontSize;
        Settings::setTrackHeaderFontSize(trackHeaderFontSize);
        emit trackHeaderFontSizeChanged();
    }
}

bool SettingsService::recordingEnabled() const
{
    return m_recordingEnabled;
}

void SettingsService::setRecordingEnabled(bool enabled)
{
    if (m_recordingEnabled != enabled) {
        m_recordingEnabled = enabled;
        Settings::setRecordingEnabled(enabled);
        emit recordingEnabledChanged();
    }
}

int SettingsService::audioBackend() const
{
    return m_audioBackend;
}

void SettingsService::setAudioBackend(int audioBackend)
{
    if (m_audioBackend != audioBackend) {
        m_audioBackend = audioBackend;
        Settings::setAudioBackend(static_cast<AudioBackend>(audioBackend));
        emit audioBackendChanged();
    }
}

bool SettingsService::multiThreadedPlaybackEnabled() const
{
    return m_multiThreadedPlaybackEnabled;
}

void SettingsService::setMultiThreadedPlaybackEnabled(bool enabled)
{
    if (m_multiThreadedPlaybackEnabled != enabled) {
        m_multiThreadedPlaybackEnabled = enabled;
        Settings::setMultiThreadedPlaybackEnabled(enabled);
        emit multiThreadedPlaybackEnabledChanged();
    }
}

bool SettingsService::jackSyncEnabled() const
{
    return m_jackSyncEnabled;
}

void SettingsService::setJackSyncEnabled(bool enabled)
{
    if (m_jackSyncEnabled != enabled) {
        m_jackSyncEnabled = enabled;
        Settings::setJackSyncEnabled(enabled);
        emit jackSyncEnabledChanged();
    }
}

bool SettingsService::jackBpmSyncEnabled() const
{
    return m_jackBpmSyncEnabled;
}

void SettingsService::setJackBpmSyncEnabled(bool enabled)
{
    if (m_jackBpmSyncEnabled != enabled) {
        m_jackBpmSyncEnabled = enabled;
        Settings::setJackBpmSyncEnabled(enabled);
        emit jackBpmSyncEnabledChanged();
    }
}

bool SettingsService::midiSyncEnabled() const
{
    return m_midiSyncEnabled;
}

void SettingsService::setMidiSyncEnabled(bool enabled)
{
    if (m_midiSyncEnabled != enabled) {
        m_midiSyncEnabled = enabled;
        Settings::setMidiSyncEnabled(enabled);
        emit midiSyncEnabledChanged();
    }
}

bool SettingsService::waveViewEnabled() const
{
    return m_waveViewEnabled;
}

void SettingsService::setWaveViewEnabled(bool enabled)
{
    if (m_waveViewEnabled != enabled) {
        m_waveViewEnabled = enabled;
        Settings::setWaveViewEnabled(enabled);
        emit waveViewEnabledChanged();
    }
}

bool SettingsService::patternPeekEnabled() const
{
    return m_patternPeekEnabled;
}

void SettingsService::setPatternPeekEnabled(bool enabled)
{
    if (m_patternPeekEnabled != enabled) {
        m_patternPeekEnabled = enabled;
        Settings::setPatternPeekEnabled(enabled);
        emit patternPeekEnabledChanged();
    }
}

bool SettingsService::midiExportForceDrumChannel10() const
{
    return m_midiExportForceDrumChannel10;
}

void SettingsService::setMidiExportForceDrumChannel10(bool enabled)
{
    if (m_midiExportForceDrumChannel10 != enabled) {
        m_midiExportForceDrumChannel10 = enabled;
        Settings::setMidiExportForceDrumChannel10(enabled);
        emit midiExportForceDrumChannel10Changed();
    }
}

bool SettingsService::midiExportAutoAssignChannels() const
{
    return m_midiExportAutoAssignChannels;
}

void SettingsService::setMidiExportAutoAssignChannels(bool enabled)
{
    if (m_midiExportAutoAssignChannels != enabled) {
        m_midiExportAutoAssignChannels = enabled;
        Settings::setMidiExportAutoAssignChannels(enabled);
        emit midiExportAutoAssignChannelsChanged();
    }
}

int SettingsService::audioBufferSize() const
{
    return m_audioBufferSize;
}

void SettingsService::setAudioBufferSize(int samples)
{
    if (m_audioBufferSize != samples) {
        m_audioBufferSize = samples;
        Settings::setAudioBufferSize(samples);
    }
}

int SettingsService::audioInputDeviceId() const
{
    return m_audioInputDeviceId;
}

void SettingsService::setAudioInputDeviceId(int deviceId)
{
    if (m_audioInputDeviceId != deviceId) {
        m_audioInputDeviceId = deviceId;
        Settings::setAudioInputDeviceId(deviceId);
    }
}

int SettingsService::audioOutputDeviceId() const
{
    return m_audioOutputDeviceId;
}

void SettingsService::setAudioOutputDeviceId(int deviceId)
{
    if (m_audioOutputDeviceId != deviceId) {
        m_audioOutputDeviceId = deviceId;
        Settings::setAudioOutputDeviceId(deviceId);
    }
}

int SettingsService::playbackOversampleFactor() const
{
    return m_playbackOversampleFactor;
}

void SettingsService::setPlaybackOversampleFactor(int factor)
{
    if (m_playbackOversampleFactor != factor) {
        m_playbackOversampleFactor = factor;
        Settings::setPlaybackOversampleFactor(factor);
        emit playbackOversampleFactorChanged();
    }
}

int SettingsService::gainStagingTargetDb() const
{
    return m_gainStagingTargetDb;
}

void SettingsService::setGainStagingTargetDb(int targetDb)
{
    if (m_gainStagingTargetDb != targetDb) {
        m_gainStagingTargetDb = targetDb;
        Settings::setGainStagingTargetDb(targetDb);
        emit gainStagingTargetDbChanged();
    }
}

int SettingsService::automationDisplayMode() const
{
    return m_automationDisplayMode;
}

void SettingsService::setAutomationDisplayMode(int mode)
{
    if (m_automationDisplayMode != mode) {
        m_automationDisplayMode = mode;
        Settings::setAutomationDisplayMode(mode);
        emit automationDisplayModeChanged();
    }
}

SettingsService::~SettingsService() = default;

} // namespace noteahead
