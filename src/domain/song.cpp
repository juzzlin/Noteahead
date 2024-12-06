// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#include "song.hpp"

#include "pattern.hpp"

namespace cacophony {

Song::Song()
{
    initialize();
}

uint32_t Song::bpm() const
{
    return m_bpm;
}

uint32_t Song::columnCount(uint32_t trackId) const
{
    return m_patterns.at(0)->columnCount(trackId);
}

uint32_t Song::lineCount(uint32_t patternId) const
{
    return m_patterns.at(patternId)->lineCount();
}

uint32_t Song::patternCount() const
{
    return static_cast<uint32_t>(m_patterns.size());
}

uint32_t Song::trackCount() const
{
    return m_patterns.at(0)->trackCount();
}

std::string Song::trackName(uint32_t trackId) const
{
    return m_patterns.at(0)->trackName(trackId);
}

void Song::setTrackName(uint32_t trackId, std::string name)
{
    m_patterns.at(0)->setTrackName(trackId, name);
}

void Song::initialize()
{
    m_patterns.clear();
    m_patterns.push_back(std::make_shared<Pattern>(64, 8));
}

} // namespace cacophony
