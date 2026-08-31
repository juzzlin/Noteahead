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

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "../common/audio_backend.hpp"

#include <cstddef>

#include <QColor>
#include <QSize>

namespace noteahead::Settings {

AudioBackend audioBackend();
void setAudioBackend(AudioBackend audioBackend);

int autoNoteOffOffset(int defaultAutoNoteOffOffset);
void setAutoNoteOffOffset(int autoNoteOffOffset);

QString controllerPort(QString defaultControllerPort);
void setControllerPort(QString controllerPort);

QSize windowSize(QSize defaultSize);
void setWindowSize(QSize size);

QStringList recentFiles();
void setRecentFiles(const QStringList & fileList);

QString lastImportDirectory();
void setLastImportDirectory(const QString & directory);

QString lastEffectImportDirectory();
void setLastEffectImportDirectory(const QString & directory);

QString lastEffectExportDirectory();
void setLastEffectExportDirectory(const QString & directory);

int step(int defaultStep);
void setStep(int step);

int velocity(int defaultVelocity);
void setVelocity(int velocity);

int visibleLines(int defaultVisibleLines);
void setVisibleLines(int visibleLines);

bool autoTrackCount();
void setAutoTrackCount(bool autoTrackCount);

int trackHeaderFontSize(int defaultTrackHeaderFontSize);
void setTrackHeaderFontSize(int trackHeaderFontSize);

bool uiUpdatesDisabledDuringPlayback();
void setUiUpdatesDisabledDuringPlayback(bool disabled);

bool recordingEnabled();
void setRecordingEnabled(bool enabled);

bool multiThreadedPlaybackEnabled();
void setMultiThreadedPlaybackEnabled(bool enabled);

bool jackSyncEnabled();
void setJackSyncEnabled(bool enabled);

bool jackBpmSyncEnabled();
void setJackBpmSyncEnabled(bool enabled);

bool midiSyncEnabled();
void setMidiSyncEnabled(bool enabled);

bool waveViewEnabled();
void setWaveViewEnabled(bool enabled);

bool patternPeekEnabled();
void setPatternPeekEnabled(bool enabled);

bool tipsEnabled();
void setTipsEnabled(bool enabled);

bool midiExportForceDrumChannel10();
void setMidiExportForceDrumChannel10(bool enabled);

bool midiExportAutoAssignChannels();
void setMidiExportAutoAssignChannels(bool enabled);

int audioBufferSize();
void setAudioBufferSize(int audioBufferSize);

int audioInputDeviceId();
void setAudioInputDeviceId(int deviceId);

int audioOutputDeviceId();
void setAudioOutputDeviceId(int deviceId);

int playbackOversampleFactor();

int automationDisplayMode(int defaultAutomationDisplayMode);
void setAutomationDisplayMode(int mode);

int automationCurveThicknessTenths(int defaultAutomationCurveThicknessTenths);
void setAutomationCurveThicknessTenths(int tenths);
void setPlaybackOversampleFactor(int factor);

int gainStagingTargetDb(int defaultGainStagingTargetDb);
void setGainStagingTargetDb(int gainStagingTargetDb);

QColor accentColor(QColor defaultAccentColor);
void setAccentColor(QColor accentColor);

QColor cursorColor(QColor defaultCursorColor);
void setCursorColor(QColor cursorColor);

int paletteAccentBlend(int defaultPaletteAccentBlend);
void setPaletteAccentBlend(int paletteAccentBlend);

//! Empty until the user has picked a language, in which case the system's UI languages apply.
QString userLanguage(QString defaultUserLanguage);
void setUserLanguage(QString userLanguage);

} // namespace noteahead::Settings

#endif // SETTINGS_HPP
