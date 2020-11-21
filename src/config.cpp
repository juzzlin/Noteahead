// This file is part of Cacophony.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
//
// Heimer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Heimer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "config.hpp"

#include <QGuiApplication>
#include <QScreen>

namespace cacophony {

Config::Config()
{
}

QSize Config::calculateDefaultWindowSize()
{
    // Detect screen dimensions
    const auto screenGeometry = QGuiApplication::primaryScreen()->geometry();
    const int height = screenGeometry.height();
    const int width = screenGeometry.width();
    return { width, height };
}

Config::~Config() = default;

} // namespace cacophony
