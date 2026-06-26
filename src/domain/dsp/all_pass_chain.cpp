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

#include "all_pass_chain.hpp"

#include <algorithm>

namespace noteahead {

void AllPassChain::setCoefficient(double coeff)
{
    m_coeff = std::clamp(coeff, -1.0, 1.0);
}

void AllPassChain::setStages(int stages)
{
    m_stages = std::clamp(stages, 1, MaxStages);
}

double AllPassChain::process(double input)
{
    double x = input;
    for (int i = 0; i < m_stages; i++) {
        const double y = m_coeff * (x - m_y1[i]) + m_x1[i];
        m_x1[i] = x;
        m_y1[i] = y;
        x = y;
    }
    return x;
}

void AllPassChain::reset()
{
    m_x1.fill(0.0);
    m_y1.fill(0.0);
}

} // namespace noteahead
