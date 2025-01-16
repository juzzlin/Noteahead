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

#include "../application/position.hpp"
#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/note_data.hpp"
#include "line.hpp"

#include <QXmlStreamWriter>

namespace cacophony {

static const auto TAG = "Column";

Column::Column(uint32_t index, uint32_t length)
  : m_index { index }
{
    initialize(length);
}

uint32_t Column::index() const
{
    return m_index;
}

void Column::initialize(uint32_t length)
{
    m_lines.clear();
    for (uint32_t i = 0; i < length; i++) {
        m_lines.push_back(std::make_shared<Line>(i));
    }
}

bool Column::hasData() const
{
    return std::find_if(m_lines.begin(), m_lines.end(), [](auto && line) {
               return line->noteData()->type() != NoteData::Type::None;
           })
      != m_lines.end();
}

uint32_t Column::lineCount() const
{
    return static_cast<uint32_t>(m_lines.size());
}

void Column::setLineCount(uint32_t lineCount)
{
    if (lineCount < m_lines.size()) {
        m_lines.erase(m_lines.begin() + lineCount, m_lines.end());
    } else if (lineCount > m_lines.size()) {
        for (uint32_t i = static_cast<uint32_t>(m_lines.size()); i < lineCount; ++i) {
            m_lines.push_back(std::make_shared<Line>(i));
        }
    }
}

void Column::addOrReplaceLine(LineS line)
{
    m_lines.at(line->index()) = line;
}

Position Column::nextNoteDataOnSameColumn(const Position & position) const
{
    auto nextNoteDataPosition = position;
    for (uint32_t line = position.line + 1; line < m_lines.size(); line++) {
        if (const auto noteData = m_lines.at(line)->noteData(); noteData->type() != NoteData::Type::None) {
            nextNoteDataPosition.line = line;
            break;
        }
    }
    return nextNoteDataPosition;
}

Position Column::prevNoteDataOnSameColumn(const Position & position) const
{
    auto nextNoteDataPosition = position;
    for (uint32_t line = 0; line < position.line; line++) {
        if (const auto noteData = m_lines.at(line)->noteData(); noteData->type() != NoteData::Type::None) {
            nextNoteDataPosition.line = line;
        }
    }
    return nextNoteDataPosition;
}

Column::NoteDataS Column::noteDataAtPosition(const Position & position) const
{
    return m_lines.at(static_cast<size_t>(position.line))->noteData();
}

void Column::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    m_lines.at(static_cast<size_t>(position.line))->setNoteData(noteData);
}

Column::EventList Column::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    EventList eventList;
    size_t tick = startTick;
    for (auto && line : m_lines) {
        if (line->noteData()->type() != NoteData::Type::None) {
            const auto event = std::make_shared<Event>(tick, line->noteData());
            eventList.push_back(event);
        }
        tick += ticksPerLine;
    }
    return eventList;
}

void Column::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyColumn());
    writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(m_index));
    writer.writeAttribute(Constants::xmlKeyLineCount(), QString::number(lineCount()));

    writer.writeStartElement(Constants::xmlKeyLines());

    for (const auto & line : m_lines) {
        if (line) {
            line->serializeToXml(writer);
        }
    }

    writer.writeEndElement(); // Lines
    writer.writeEndElement(); // Column
}

} // namespace cacophony
