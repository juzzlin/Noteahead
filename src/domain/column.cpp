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

#include "column.hpp"

#include "line.hpp"

namespace cacophony {

Column::Column(uint32_t length)
{
    initialize(length);
}

void Column::initialize(uint32_t length)
{
    m_lines.clear();
    for (uint32_t i = 0; i < length; i++) {
        m_lines.push_back(std::make_shared<Line>());
    }
}

} // namespace cacophony
