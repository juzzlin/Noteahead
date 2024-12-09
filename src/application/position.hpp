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

#ifndef POSITION_HPP
#define POSITION_HPP

#include <QObject>
#include <cstdint>

namespace cacophony {

struct Position
{
    Q_GADGET
    Q_PROPERTY(uint32_t pattern MEMBER pattern)
    Q_PROPERTY(uint32_t track MEMBER track)
    Q_PROPERTY(uint32_t column MEMBER column)
    Q_PROPERTY(int line MEMBER line)

public:
    uint32_t pattern = 0;

    uint32_t track = 0;

    uint32_t column = 0;

    int line = 0;
};

} // namespace cacophony

#endif // POSITION_HPP
