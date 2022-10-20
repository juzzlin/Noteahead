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

#include "load_meter.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

void LoadMeter::setActive(bool active)
{
    m_active.store(active);
    if (!active) {
        reset();
    }
}

bool LoadMeter::active() const
{
    return m_active.load();
}

void LoadMeter::addBlock(std::chrono::nanoseconds elapsed, double bufferSeconds)
{
    if (!m_active.load() || bufferSeconds <= 0.0) {
        return;
    }

    const double seconds = std::chrono::duration<double>(elapsed).count();
    const auto percent = static_cast<float>(100.0 * seconds / bufferSeconds);

    const float smoothing = std::min(1.0f, static_cast<float>(bufferSeconds) / WindowSeconds);
    const float load = m_load.load();
    m_load.store(load + smoothing * (percent - load));

    const float fallback = PeakFallbackPerSecond * static_cast<float>(bufferSeconds);
    m_peak.store(std::max(percent, m_peak.load() - fallback));

    if (percent > 100.0f) {
        m_overruns.fetch_add(1);
    }
}

float LoadMeter::loadPercent() const
{
    return m_load.load();
}

float LoadMeter::peakPercent() const
{
    return m_peak.load();
}

uint64_t LoadMeter::overrunCount() const
{
    return m_overruns.load();
}

void LoadMeter::reset()
{
    m_load.store(0.0f);
    m_peak.store(0.0f);
    m_overruns.store(0);
}

} // namespace noteahead
