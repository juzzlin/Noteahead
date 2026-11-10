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

#ifndef XY_PAD_TRAIL_HPP
#define XY_PAD_TRAIL_HPP

#include <QPointF>

#include <chrono>
#include <deque>
#include <vector>

namespace noteahead {

//! The recent path of an XY pad, for the fading tail drawn behind it. Points are kept in the pad's
//! own -1..1 coordinates, so the tail keeps its shape when the pad is resized, and each is dropped
//! once it is older than the lifetime.
class XyPadTrail
{
public:
    using Clock = std::chrono::steady_clock;

    //! How long a point stays on the tail.
    static constexpr std::chrono::milliseconds Lifetime { 600 };
    //! Most points kept: at the display rate, more than a lifetime's worth.
    static constexpr size_t MaxPoints { 64 };

    struct Point
    {
        QPointF position;
        //! 0 for a point just added, 1 for one at the end of its life.
        double age = 0.0;
    };

    //! Adds where the pad is at @p time. A pad that has not moved adds nothing, so a still pad's
    //! tail fades away behind it.
    void add(const QPointF & position, Clock::time_point time);
    void clear();

    //! The points still alive at @p time, oldest first.
    std::vector<Point> points(Clock::time_point time) const;
    bool isEmpty(Clock::time_point time) const;

private:
    void expire(Clock::time_point time);

    struct Sample
    {
        QPointF position;
        Clock::time_point time;
    };

    std::deque<Sample> m_samples;
};

} // namespace noteahead

#endif // XY_PAD_TRAIL_HPP
