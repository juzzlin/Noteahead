// This file is part of Cacophony.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "settings.hpp"

#include <QSettings>

namespace cacophony {

const auto settingsGroupEditor = "Editor";
const auto settingsGroupMainWindow = "MainWindow";
const auto stepKey = "step";
const auto velocityKey = "velocity";
const auto windowSizeKey = "size";

QSize settings::loadWindowSize(QSize defaultSize)
{
    QSettings settings;
    settings.beginGroup(settingsGroupMainWindow);
    const auto size = settings.value(windowSizeKey, defaultSize).toSize();
    settings.endGroup();
    return size;
}

void settings::saveWindowSize(QSize size)
{
    QSettings settings;
    settings.beginGroup(settingsGroupMainWindow);
    settings.setValue(windowSizeKey, size);
    settings.endGroup();
}

int settings::loadStep(int defaultStep)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    const auto step = settings.value(stepKey, defaultStep).toInt();
    settings.endGroup();
    return step;
}

void settings::saveStep(int step)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    settings.setValue(stepKey, step);
    settings.endGroup();
}

int settings::loadVelocity(int defaultVelocity)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    const auto velocity = settings.value(velocityKey, defaultVelocity).toInt();
    settings.endGroup();
    return velocity;
}

void settings::saveVelocity(int velocity)
{
    QSettings settings;
    settings.beginGroup(settingsGroupEditor);
    settings.setValue(velocityKey, velocity);
    settings.endGroup();
}

} // namespace cacophony
