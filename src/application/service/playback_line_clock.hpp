// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef PLAYBACK_LINE_CLOCK_HPP
#define PLAYBACK_LINE_CLOCK_HPP

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <utility>

namespace noteahead {

class Song;

//! Which line of the song is being heard at a given moment, for whatever records a live gesture
//! against it: the lines playback announces, each with the time it is heard, a lookahead after it
//! is announced.
class PlaybackLineClock
{
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    //! Line @p line of pattern @p pattern is heard at @p due.
    void announce(size_t pattern, size_t line, TimePoint due);
    void clear();

    //! The pattern and the line, between lines, being heard at @p time. Nothing before the first
    //! announced line has been heard.
    std::optional<std::pair<size_t, double>> songLine(const Song & song, TimePoint time) const;

private:
    struct Anchor
    {
        size_t pattern = 0;
        size_t line = 0;
        TimePoint due;
    };

    //! A few, since they are heard a lookahead after they are announced
    std::deque<Anchor> m_anchors;
};

} // namespace noteahead

#endif // PLAYBACK_LINE_CLOCK_HPP
