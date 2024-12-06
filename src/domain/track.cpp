// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "track.hpp"

#include "column.hpp"

namespace cacophony {

Track::Track(std::string name, Type type, uint32_t length, uint32_t columnCount)
  : m_name { name }
  , m_type { type }
{
    initialize(length, columnCount);
}

void Track::initialize(uint32_t length, uint32_t columnCount)
{
    m_columns.clear();
    for (uint32_t column = 0; column < columnCount; column++) {
        m_columns.push_back(std::make_shared<Column>(length));
    }
}

void Track::setName(const std::string & name)
{
    m_name = name;
}

uint32_t Track::columnCount() const
{
    return static_cast<uint32_t>(m_columns.size());
}

uint32_t Track::lineCount() const
{
    return m_columns.at(0)->lineCount();
}

std::string Track::name() const
{
    return m_name;
}

} // namespace cacophony
