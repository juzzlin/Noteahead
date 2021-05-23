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

#include "panning_effect.hpp"

#include <algorithm>

namespace noteahead {

void PanningEffect::setPan(float pan)
{
    m_pan = pan;
}

void PanningEffect::process(double & left, double & right)
{
    const double gainL = std::min(1.0, 2.0 - static_cast<double>(m_pan) * 2.0);
    const double gainR = std::min(1.0, static_cast<double>(m_pan) * 2.0);
    left *= gainL;
    right *= gainR;
}

} // namespace noteahead
