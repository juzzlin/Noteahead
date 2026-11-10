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

#include "playback_line_clock.hpp"

#include "../../domain/tracker/song.hpp"

#include <algorithm>

namespace noteahead {

namespace {
//! How many announced lines are kept: far more than any lookahead spans.
constexpr size_t MaxAnchors { 16 };
} // namespace

void PlaybackLineClock::announce(size_t pattern, size_t line, TimePoint due)
{
    m_anchors.push_back({ pattern, line, due });
    while (m_anchors.size() > MaxAnchors) {
        m_anchors.pop_front();
    }
}

void PlaybackLineClock::clear()
{
    m_anchors.clear();
}

std::optional<std::pair<size_t, double>> PlaybackLineClock::songLine(const Song & song, TimePoint time) const
{
    if (!song.beatsPerMinute() || !song.linesPerBeat()) {
        return std::nullopt;
    }
    const auto lineDuration = std::chrono::duration<double>(60.0 / static_cast<double>(song.beatsPerMinute() * song.linesPerBeat()));
    // The last line already heard, and how far into it the song is, short of the next one
    for (auto anchor = m_anchors.rbegin(); anchor != m_anchors.rend(); anchor++) {
        if (anchor->due <= time) {
            const double into = std::chrono::duration<double>(time - anchor->due) / lineDuration;
            return std::pair { anchor->pattern, static_cast<double>(anchor->line) + std::clamp(into, 0.0, 0.999) };
        }
    }
    return std::nullopt;
}

} // namespace noteahead
