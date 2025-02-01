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

#ifndef COPY_MANAGER_HPP
#define COPY_MANAGER_HPP

#include <memory>
#include <vector>

#include "../domain/note_data.hpp"
#include "position.hpp"

namespace noteahead {

class Pattern;

class CopyManager
{
public:
    CopyManager();

    ~CopyManager();

    using PositionList = std::vector<Position>;
    PositionList pushSourcePattern(const Pattern & pattern);

    PositionList pushSourceTrack(const Pattern & pattern, size_t trackIndex);

    PositionList pushSourceColumn(const Pattern & pattern, size_t trackIndex, size_t columnIndex);

    using PatternS = std::shared_ptr<Pattern>;
    PositionList pasteCopiedData(PatternS targetPattern);

private:
    PatternS m_sourcePattern;

    std::vector<std::pair<Position, NoteData>> m_copiedData;
};

} // namespace noteahead

#endif // COPY_MANAGER_HPP
