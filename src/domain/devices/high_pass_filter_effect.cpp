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

#include "high_pass_filter_effect.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

void HighPassFilterEffect::setCutoff(float cutoff)
{
    m_cutoff = cutoff;
}
void HighPassFilterEffect::process(float & left, float & right, uint32_t sampleRate)
{
    if (m_cutoff <= 0.001f) {
        return;
    }

    // Zero-Delay Feedback State Variable Filter (2nd order)
    const float freq = 20.0f * std::pow(std::min(20000.0f, sampleRate * 0.49f) / 20.0f, m_cutoff);
    const double g = std::tan(std::numbers::pi * static_cast<double>(freq) / static_cast<double>(sampleRate));
    const double k = 1.0; // Q = 1.0 / k
    const double damping = 1.0 / (1.0 + g * (g + k));

    // Left channel
    {
        const double hp = (static_cast<double>(left) - (g + k) * m_s1L - m_s2L) * damping;
        const double v1 = g * hp;
        const double v = v1 + m_s1L;
        m_s1L = v1 + v;
        const double v2 = g * v;
        const double lp = v2 + m_s2L;
        m_s2L = v2 + lp;
        left = static_cast<float>(hp);
    }

    // Right channel
    {
        const double hp = (static_cast<double>(right) - (g + k) * m_s1R - m_s2R) * damping;
        const double v1 = g * hp;
        const double v = v1 + m_s1R;
        m_s1R = v1 + v;
        const double v2 = g * v;
        const double lp = v2 + m_s2R;
        m_s2R = v2 + lp;
        right = static_cast<float>(hp);
    }

    // NaN protection
    if (std::isnan(left) || std::isnan(right)) {
        reset();
        left = 0.0f;
        right = 0.0f;
    }
}

void HighPassFilterEffect::reset()
{
    m_s1L = m_s2L = 0.0;
    m_s1R = m_s2R = 0.0;
}

} // namespace noteahead
