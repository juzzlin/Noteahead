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

namespace noteahead {

void PanningEffect::setPan(float pan)
{
    m_panner.setPan(static_cast<double>(pan));
}

void PanningEffect::process(double & left, double & right)
{
    m_panner.process(left, right);
}

std::string PanningEffect::typeIdString()
{
    return "c3d4e5f6-a7b8-4c9d-0e1f-2a3b4c5d6e7f";
}

std::string PanningEffect::type() const
{
    return "panning";
}

std::string PanningEffect::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
