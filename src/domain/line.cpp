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

#include "line.hpp"

#include "../common/constants.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "Line";

Line::Line(size_t index)
  : m_index { index }
  , m_noteData { std::make_unique<NoteData>() }
{
}

Line::Line(size_t index, const NoteData & noteData)
  : m_index { index }
  , m_noteData { std::make_unique<NoteData>(noteData) }
{
}

size_t Line::index() const
{
    return m_index;
}

void Line::setIndex(size_t index)
{
    m_index = index;
}

void Line::clear()
{
    setNoteData({});
}

void Line::setNoteData(const NoteData & noteData)
{
    juzzlin::L(TAG).debug() << "Set note data " << noteData.toString();
    *m_noteData = noteData;
}

Line::NoteDataS Line::noteData() const
{
    return m_noteData;
}

void Line::serializeToXml(QXmlStreamWriter & writer) const
{
    if (m_noteData && m_noteData->type() != NoteData::Type::None) {
        writer.writeStartElement(Constants::xmlKeyLine());
        writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(m_index));
        m_noteData->serializeToXml(writer);
        writer.writeEndElement(); // Line
    }
}

} // namespace noteahead
