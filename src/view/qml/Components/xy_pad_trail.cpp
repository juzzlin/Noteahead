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

#include "xy_pad_trail.hpp"

#include <algorithm>

namespace noteahead {

void XyPadTrail::add(const QPointF & position, Clock::time_point time)
{
    expire(time);
    if (!m_samples.empty()) {
        const auto & last = m_samples.back();
        if (last.position == position) {
            return;
        }
        if (m_samples.size() == 1 && time - last.time >= Lifetime) {
            // The pad had stood still since the last point, so the move starts from there now
            m_samples.back().time = time;
        }
    }
    m_samples.push_back({ position, time });
    while (m_samples.size() > MaxPoints) {
        m_samples.pop_front();
    }
}

void XyPadTrail::clear()
{
    m_samples.clear();
}

void XyPadTrail::expire(Clock::time_point time)
{
    // The newest point stays as long as the pad is there: it is where the next move starts from
    while (m_samples.size() > 1 && time - m_samples.front().time >= Lifetime) {
        m_samples.pop_front();
    }
}

std::vector<XyPadTrail::Point> XyPadTrail::points(Clock::time_point time) const
{
    std::vector<Point> result;
    for (auto && sample : m_samples) {
        const double age = std::chrono::duration<double>(time - sample.time) / std::chrono::duration<double>(Lifetime);
        if (age < 1.0) {
            result.push_back({ sample.position, std::clamp(age, 0.0, 1.0) });
        }
    }
    return result;
}

bool XyPadTrail::isEmpty(Clock::time_point time) const
{
    return points(time).size() < 2;
}

} // namespace noteahead
