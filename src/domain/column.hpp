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

#ifndef COLUMN_HPP
#define COLUMN_HPP

#include <memory>
#include <vector>

namespace cacophony {

class Line;
struct NoteData;

class Column
{
public:
    Column(uint32_t length);

    using NoteDataS = std::shared_ptr<NoteData>;

    uint32_t lineCount() const;

    NoteDataS noteDataAtPosition(uint32_t position) const;

private:
    void initialize(uint32_t length);

    std::vector<std::shared_ptr<Line>> m_lines;
};

} // namespace cacophony

#endif // COLUMN_HPP
