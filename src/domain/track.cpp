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

#include "../application/position.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/note_data.hpp"
#include "../domain/track.hpp"
#include "column.hpp"

#include <QXmlStreamWriter>

namespace cacophony {

static const auto TAG = "Track";

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

std::string Track::name() const
{
    return m_name;
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

Track::NoteDataS Track::noteDataAtPosition(const Position & position) const
{
    return m_columns.at(position.column)->noteDataAtPosition(position);
}

void Track::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    m_columns.at(position.column)->setNoteDataAtPosition(noteData, position);
}

Track::EventList Track::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    Track::EventList eventList;
    for (auto && column : m_columns) {
        const auto columnList = column->renderToEvents(startTick, ticksPerLine);
        std::copy(columnList.begin(), columnList.end(), std::back_inserter(eventList));
    }
    return eventList;
}

void Track::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement("Track");

    writer.writeTextElement("Name", QString::fromStdString(m_name));
    writer.writeTextElement("Type", QString::number(static_cast<int>(m_type)));
    writer.writeTextElement("LineCount", QString::number(lineCount()));
    writer.writeTextElement("ColumnCount", QString::number(columnCount()));

    writer.writeStartElement("Columns");
    for (const auto & column : m_columns) {
        if (column) {
            column->serializeToXml(writer);
        }
    }
    writer.writeEndElement(); // Columns

    writer.writeEndElement(); // Track
}

} // namespace cacophony
