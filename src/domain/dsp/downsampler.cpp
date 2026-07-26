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

#include "downsampler.hpp"

namespace noteahead {

uint8_t clampOversampleFactor(uint8_t factor)
{
    return (factor == 1 || factor == 2 || factor == 4) ? factor : 2;
}

float Downsampler::process(const float * highRate, uint8_t factor)
{
    switch (factor) {
    case 2:
        return m_stage1.process(highRate[0], highRate[1]);
    case 4: {
        // First stage decimates 4x -> 2x (two calls), second stage decimates 2x -> 1x.
        const float a = m_stage1.process(highRate[0], highRate[1]);
        const float b = m_stage1.process(highRate[2], highRate[3]);
        return m_stage2.process(a, b);
    }
    case 1:
    default:
        return highRate[0];
    }
}

void Downsampler::reset()
{
    m_stage1.reset();
    m_stage2.reset();
}

} // namespace noteahead
