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

#include "dc_blocker.hpp"

#include <cmath>
#include <numbers>

namespace noteahead {

double DcBlocker::process(double input)
{
    if (m_sampleRate != m_lastSampleRate) {
        updateCoefficient();
    }

    const double output = input - m_x1 + m_coeff * m_y1;
    m_x1 = input;
    m_y1 = output;
    return output;
}

void DcBlocker::reset()
{
    m_x1 = m_y1 = 0.0;
}

void DcBlocker::updateCoefficient()
{
    m_coeff = std::exp(-2.0 * std::numbers::pi * 5.0 / m_sampleRate);
    m_lastSampleRate = m_sampleRate;
}

} // namespace noteahead
