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

#include "config.hpp"
#include "../infra/settings.hpp"

#include <QGuiApplication>
#include <QScreen>

namespace noteahead {

Config::Config()
  : m_visibleLines { settings::visibleLines(32) }
{
}

int Config::autoNoteOffOffset() const
{
    return settings::autoNoteOffOffset(250);
}

void Config::setAutoNoteOffOffset(int autoNoteOffOffset)
{
    settings::setAutoNoteOffOffset(autoNoteOffOffset);
}

QSize Config::windowSize(QSize defaultSize) const
{
    return settings::windowSize(defaultSize);
}

void Config::setWindowSize(QSize size)
{
    settings::setWindowSize(size);
}

int Config::step(int defaultStep) const
{
    return settings::step(defaultStep);
}

void Config::setStep(int step)
{
    settings::setStep(step);
}

int Config::velocity(int defaultVelocity) const
{
    return settings::velocity(defaultVelocity);
}

void Config::setVelocity(int velocity)
{
    settings::setVelocity(velocity);
}

int Config::visibleLines() const
{
    return m_visibleLines;
}

void Config::setVisibleLines(int visibleLines)
{
    if (m_visibleLines != visibleLines) {
        m_visibleLines = visibleLines;
        settings::setVisibleLines(visibleLines);
        emit visibleLinesChanged();
    }
}

Config::~Config() = default;

} // namespace noteahead
