// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef LINE_HPP
#define LINE_HPP

#include "note_data.hpp"

#include <memory>

class QXmlStreamWriter;

namespace noteahead {

class Line
{
public:
    explicit Line(size_t index);

    Line(size_t index, const NoteData & noteData);

    size_t index() const;

    void setIndex(size_t index);

    void clear();

    using NoteDataS = std::shared_ptr<NoteData>;

    NoteDataS noteData() const;

    void setNoteData(const NoteData & noteData);

    void serializeToXml(QXmlStreamWriter & writer) const;

private:
    size_t m_index = 0;

    NoteDataS m_noteData;
};

} // namespace noteahead

#endif // LINE_HPP
