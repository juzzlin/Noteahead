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

#include "clip_detector.hpp"

#include <cmath>

namespace noteahead {

void ClipDetector::write(const double * interleavedStereo, uint32_t frameCount)
{
    if (!interleavedStereo || !frameCount || m_clipped.load(std::memory_order_relaxed)) {
        return;
    }

    const uint32_t sampleCount = frameCount * 2;
    for (uint32_t i = 0; i < sampleCount; i++) {
        if (std::abs(interleavedStereo[i]) >= Threshold) {
            m_clipped.store(true, std::memory_order_relaxed);
            return;
        }
    }
}

bool ClipDetector::clipped() const
{
    return m_clipped.load(std::memory_order_relaxed);
}

void ClipDetector::clear()
{
    m_clipped.store(false, std::memory_order_relaxed);
}

} // namespace noteahead
