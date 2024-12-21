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

#include "config.hpp"
#include "../infra/settings.hpp"

#include <QGuiApplication>
#include <QScreen>

namespace cacophony {

Config::Config()
{
}

QSize Config::loadWindowSize(QSize defaultSize) const
{
    return settings::loadWindowSize(defaultSize);
}

void Config::saveWindowSize(QSize size)
{
    settings::saveWindowSize(size);
}

int Config::loadStep(int defaultStep) const
{
    return settings::loadStep(defaultStep);
}

void Config::saveStep(int step)
{
    settings::saveStep(step);
}

int Config::loadVelocity(int defaultVelocity) const
{
    return settings::loadVelocity(defaultVelocity);
}

void Config::saveVelocity(int velocity)
{
    settings::saveVelocity(velocity);
}

Config::~Config() = default;

} // namespace cacophony
