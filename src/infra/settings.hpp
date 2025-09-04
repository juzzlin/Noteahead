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

#include <QSize>

namespace noteahead::Settings {

int autoNoteOffOffset(int defaultAutoNoteOffOffset);
void setAutoNoteOffOffset(int autoNoteOffOffset);

QString controllerPort(QString defaultControllerPort);
void setControllerPort(QString controllerPort);

QSize windowSize(QSize defaultSize);
void setWindowSize(QSize size);

QStringList recentFiles();
void setRecentFiles(const QStringList & fileList);

int step(int defaultStep);
void setStep(int step);

int velocity(int defaultVelocity);
void setVelocity(int velocity);

int visibleLines(int defaultVisibleLines);
void setVisibleLines(int visibleLines);

bool recordingEnabled();
void setRecordingEnabled(bool enabled);

} // namespace noteahead::Settings

#endif // SETTINGS_HPP
