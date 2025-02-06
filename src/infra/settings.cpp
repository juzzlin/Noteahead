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

#include "settings.hpp"

#include <QSettings>

namespace noteahead::settings {

const auto recentFilesArrayKey = "recentFilesArray";

const auto recentFilesFilePathKey = "filePath";

const auto settingsGroupEditor = "Editor";

const auto settingsGroupMainWindow = "MainWindow";

const auto stepKey = "step";

const auto velocityKey = "velocity";

const auto visibleLinesKey = "visibleLines";

const auto windowSizeKey = "size";

QSize windowSize(QSize defaultSize)
{
    QSettings settings;
    settings.beginGroup(settingsGroupMainWindow);
    const auto size = settings.value(windowSizeKey, defaultSize).toSize();
    settings.endGroup();
    return size;
}

void setWindowSize(QSize size)
{
    QSettings settings;
    settings.beginGroup(settingsGroupMainWindow);
    settings.setValue(windowSizeKey, size);
    settings.endGroup();
}

QStringList recentFiles()
{
    QStringList fileList;
    QSettings settings;
    const int size = settings.beginReadArray(recentFilesArrayKey);
    for (int i = 0; i < size; i++) {
        settings.setArrayIndex(i);
        fileList.push_back(settings.value(recentFilesFilePathKey).toString());
    }
    settings.endArray();
    return fileList;
}

void setRecentFiles(const QStringList & fileList)
{
    QSettings settings;
    settings.beginWriteArray(recentFilesArrayKey);
    for (int i = 0; i < fileList.size(); i++) {
        settings.setArrayIndex(i);
        settings.setValue(recentFilesFilePathKey, fileList.at(i));
    }
    settings.endArray();
}

int step(int defaultStep)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    const auto step = settings.value(stepKey, defaultStep).toInt();
    settings.endGroup();
    return step;
}

void setStep(int step)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    settings.setValue(stepKey, step);
    settings.endGroup();
}

int velocity(int defaultVelocity)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    const auto velocity = settings.value(velocityKey, defaultVelocity).toInt();
    settings.endGroup();
    return velocity;
}

void setVelocity(int velocity)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    settings.setValue(velocityKey, velocity);
    settings.endGroup();
}

int visibleLines(int defaultVisibleLines)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    const auto visibleLines = settings.value(visibleLinesKey, defaultVisibleLines).toInt();
    settings.endGroup();
    return visibleLines;
}

void setVisibleLines(int visibleLines)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    settings.setValue(visibleLinesKey, visibleLines);
    settings.endGroup();
}

} // namespace noteahead::settings
