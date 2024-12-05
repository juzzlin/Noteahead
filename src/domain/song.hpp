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

#ifndef SONG_HPP
#define SONG_HPP

#include <memory>
#include <vector>

namespace cacophony {

class Pattern;

class Song
{
public:
    Song();

    uint32_t bpm() const;

    using PatternS = std::shared_ptr<Pattern>;

    uint32_t trackCount() const;

private:
    void initialize();

    uint32_t m_bpm = 120;

    std::vector<PatternS> m_patterns;
};

} // namespace cacophony

#endif // SONG_HPP
