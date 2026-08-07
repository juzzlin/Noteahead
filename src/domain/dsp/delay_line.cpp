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

#include "delay_line.hpp"

#include <algorithm>

namespace noteahead {

void DelayLine::setMaxDelay(size_t maxSamples)
{
    m_buffer.assign(maxSamples, 0.0);
    m_writePos = 0;
    m_delay = 0;
    m_fraction = 0.0;
}

void DelayLine::setDelay(size_t samples)
{
    m_delay = m_buffer.empty() ? 0 : std::min(samples, m_buffer.size());
    m_fraction = 0.0;
}

void DelayLine::setFractionalDelay(double samples)
{
    // The interpolating read needs one extra tap behind the integer one, so the
    // usable range is one sample shorter than the buffer.
    if (m_buffer.size() < 2) {
        m_delay = 0;
        m_fraction = 0.0;
        return;
    }
    const auto clamped = std::clamp(samples, 1.0, static_cast<double>(m_buffer.size() - 1));
    m_delay = static_cast<size_t>(clamped);
    m_fraction = clamped - static_cast<double>(m_delay);
}

void DelayLine::write(double sample)
{
    if (m_buffer.empty())
        return;
    m_buffer[m_writePos] = sample;
    m_writePos = (m_writePos + 1) % m_buffer.size();
}

double DelayLine::read() const
{
    if (m_buffer.empty() || m_delay == 0)
        return 0.0;
    const size_t n = m_buffer.size();
    const double s0 = m_buffer[(m_writePos + n - m_delay) % n];
    if (m_fraction == 0.0)
        return s0;
    // One tap further back; setFractionalDelay guarantees m_delay + 1 <= n.
    const double s1 = m_buffer[(m_writePos + n - m_delay - 1) % n];
    return s0 + (s1 - s0) * m_fraction;
}

void DelayLine::reset()
{
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0);
    m_writePos = 0;
}

size_t DelayLine::delay() const
{
    return m_delay;
}

double DelayLine::fraction() const
{
    return m_fraction;
}

size_t DelayLine::capacity() const
{
    return m_buffer.size();
}

} // namespace noteahead
