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

#ifndef TRACK_HPP
#define TRACK_HPP

#include <memory>
#include <vector>

namespace cacophony {

class Column;
class Instrument;

class Track
{
public:
    enum class Type
    {
        Drum,
        Note,
        Chord,
        Control
    };

    explicit Track(Type type, uint32_t length, uint32_t columnCount);

private:
    void initialize(uint32_t length, uint32_t columnCount);

    Type m_type;

    std::vector<std::shared_ptr<Column>> m_columns;

    std::shared_ptr<Instrument> m_instrument;
};

} // namespace cacophony

#endif // TRACK_HPP
