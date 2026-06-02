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

#include "volume_effect.hpp"

namespace noteahead {

void VolumeEffect::setVolume(float volume)
{
    m_volume = volume;
}

void VolumeEffect::process(double & left, double & right)
{
    left *= m_volume;
    right *= m_volume;
}

std::string VolumeEffect::typeIdString()
{
    return "d4e5f6a7-b8c9-4d0e-1f2a-3b4c5d6e7f8a";
}

std::string VolumeEffect::type() const
{
    return "volume";
}

std::string VolumeEffect::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
