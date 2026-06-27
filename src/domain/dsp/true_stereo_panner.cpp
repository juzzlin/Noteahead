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

#include "true_stereo_panner.hpp"

#include <cmath>
#include <numbers>

namespace noteahead {

void TrueStereoPanner::setPan(double pan)
{
    m_pan = pan;
}

void TrueStereoPanner::setWidth(double width)
{
    m_width = width;
}

double TrueStereoPanner::pan() const
{
    return m_pan;
}

double TrueStereoPanner::width() const
{
    return m_width;
}

void TrueStereoPanner::processMono(double mono, double & left, double & right) const
{
    const double angle = m_pan * std::numbers::pi * 0.5;
    left = mono * std::cos(angle);
    right = mono * std::sin(angle);
}

void TrueStereoPanner::process(double & left, double & right) const
{
    const double mid = (left + right) * 0.5;
    const double side = (left - right) * 0.5;
    left = mid + side * m_width;
    right = mid - side * m_width;

    const double angle = m_pan * std::numbers::pi * 0.5;
    left *= std::cos(angle);
    right *= std::sin(angle);
}

} // namespace noteahead
