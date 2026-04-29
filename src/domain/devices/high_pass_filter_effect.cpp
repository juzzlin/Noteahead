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

    const float freq = 20.0f * std::pow(std::min(20000.0f, sampleRate * 0.49f) / 20.0f, m_cutoff);
    const float f = 2.0f * std::sin(std::numbers::pi_v<float> * freq / static_cast<float>(sampleRate));
    const float q = 0.5f;

    m_hpL = left - m_lpL - q * m_bpL;
    m_bpL += f * m_hpL;
    m_lpL += f * m_bpL;
    left = m_hpL;

    m_hpR = right - m_lpR - q * m_bpR;
    m_bpR += f * m_hpR;
    m_lpR += f * m_bpR;
    right = m_hpR;
}

void HighPassFilterEffect::reset()
{
    m_lpL = m_hpL = m_bpL = 0.0f;
    m_lpR = m_hpR = m_bpR = 0.0f;
}

} // namespace noteahead
