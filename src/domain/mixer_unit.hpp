
// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIXER_UNIT_HPP
#define MIXER_UNIT_HPP

#include <cstddef>
#include <string>

namespace noteahead {

class MixerUnit
{
public:
    MixerUnit(size_t index, std::string name);

    virtual ~MixerUnit();

    size_t index() const;

    std::string name() const;
    virtual void setName(const std::string & name);

private:
    size_t m_index = 0;

    std::string m_name;
};

} // namespace noteahead

#endif // MIXER_UNIT_HPP
