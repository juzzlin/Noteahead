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

#include "volume.hpp"

namespace noteahead {

void Volume::setVolume(float volume)
{
    m_volume = volume;
}

void Volume::processSample(double & left, double & right)
{
    left *= m_volume;
    right *= m_volume;
}

std::string Volume::typeIdString()
{
    return "36b7f768-e420-446f-af25-0c4bd33e89c8";
}

std::string Volume::type() const
{
    return "volume";
}

std::string Volume::typeId() const
{
    return typeIdString();
}

} // namespace noteahead
