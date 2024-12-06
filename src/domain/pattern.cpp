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

#include "pattern.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "track.hpp"

namespace cacophony {

static const auto TAG = "Pattern";

Pattern::Pattern(uint32_t length, uint32_t trackCount)
{
    initialize(length, trackCount);
}

uint32_t Pattern::columnCount(uint32_t trackId) const
{
    return m_tracks.at(trackId)->columnCount();
}

uint32_t Pattern::lineCount() const
{
    return m_tracks.at(0)->lineCount();
}

uint32_t Pattern::trackCount() const
{
    return static_cast<uint32_t>(m_tracks.size());
}

std::string Pattern::trackName(uint32_t trackId) const
{
    return m_tracks.at(trackId)->name();
}

void Pattern::setTrackName(uint32_t trackId, std::string name)
{
    juzzlin::L(TAG).info() << "Changing name of track " << trackId << " from " << trackName(trackId) << " to " << name;
    m_tracks.at(trackId)->setName(name);
}

void Pattern::initialize(uint32_t length, uint32_t trackCount)
{
    for (uint32_t i = 0; i < trackCount; i++) {
        m_tracks.push_back(std::make_shared<Track>("Track " + std::to_string(i), Track::Type::Drum, length, 1));
    }
}

} // namespace cacophony
