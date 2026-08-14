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

#include "pan_aware_width.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void PanAwareWidth::process(std::span<double> buffer, uint32_t frameCount, double width, double pan)
{
    width = std::clamp(width, 0.0, 1.0);
    const double angle = std::clamp(pan, 0.0, 1.0) * std::numbers::pi * 0.5;
    const double leftGain = std::cos(angle);
    const double rightGain = std::sin(angle);

    const double startWidth = m_width.value_or(width);
    const double startLeftGain = m_width ? m_leftGain : leftGain;
    const double startRightGain = m_width ? m_rightGain : rightGain;
    m_width = width;
    m_leftGain = leftGain;
    m_rightGain = rightGain;

    if (startWidth >= 1.0 && width >= 1.0) {
        return;
    }

    frameCount = std::min(frameCount, static_cast<uint32_t>(buffer.size() / 2));
    const double step = frameCount ? 1.0 / static_cast<double>(frameCount) : 0.0;
    for (uint32_t i = 0; i < frameCount; i++) {
        const double t = static_cast<double>(i + 1) * step;
        const double w = startWidth + (width - startWidth) * t;
        const double gl = startLeftGain + (leftGain - startLeftGain) * t;
        const double gr = startRightGain + (rightGain - startRightGain) * t;
        double & left = buffer[i * 2];
        double & right = buffer[i * 2 + 1];
        // The part of the signal that lies along the pan direction
        const double folded = (1.0 - w) * (gl * left + gr * right);
        left = w * left + gl * folded;
        right = w * right + gr * folded;
    }
}

void PanAwareWidth::reset()
{
    m_width.reset();
}

} // namespace noteahead
