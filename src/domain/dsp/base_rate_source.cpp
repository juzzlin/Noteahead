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

#include "base_rate_source.hpp"

namespace noteahead {

void BaseRateSource::setOversampleFactor(uint8_t factor)
{
    if (const auto clamped = clampOversampleFactor(factor); clamped != m_factor) {
        m_factor = clamped;
        reset();
    }
}

void BaseRateSource::reset()
{
    m_upsampler.reset();
    m_interpolated.fill(0.0f);
    m_index = 0;
}

} // namespace noteahead
