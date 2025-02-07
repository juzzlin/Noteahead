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

#include "column.hpp"

#include "../application/position.hpp"
#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/note_data.hpp"
#include "line.hpp"

#include <QXmlStreamWriter>

#include <algorithm>

namespace noteahead {

static const auto TAG = "Column";

Column::Column(size_t index, size_t length)
  : m_index { index }
{
    initialize(length);
}

size_t Column::index() const
{
    return m_index;
}

void Column::initialize(size_t length)
{
    m_lines.clear();
    for (size_t i = 0; i < length; i++) {
        m_lines.push_back(std::make_shared<Line>(i));
    }
    m_virtualLineCount = m_lines.size();
}

bool Column::hasData() const
{
    return std::ranges::find_if(m_lines, [](auto && line) {
               return line->noteData()->type() != NoteData::Type::None;
           })
      != m_lines.end();
}

bool Column::hasPosition(const Position & position) const
{
    return position.column == m_index && position.line < m_virtualLineCount;
}

size_t Column::lineCount() const
{
    return m_virtualLineCount;
}

void Column::setLineCount(size_t lineCount)
{
    m_virtualLineCount = lineCount;
    if (m_virtualLineCount > m_lines.size()) {
        m_lines.reserve(m_virtualLineCount);
        for (size_t i = m_lines.size(); i < m_virtualLineCount; i++) {
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
    for (size_t line = position.line + 1; line < m_lines.size(); line++) {
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
    for (size_t line = 0; line < position.line; line++) {
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

using PositionList = std::vector<Position>;
Column::PositionList Column::deleteNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Delete note data at position: " << noteData.toString() << " @ " << position.toString();
    Column::PositionList changedPositions;
    for (size_t i = position.line; i < m_lines.size(); i++) {
        auto changedPosition = position;
        changedPosition.line = i;
        changedPositions.push_back(changedPosition);
        if (i + 1 < m_lines.size()) {
            m_lines.at(i)->setNoteData(*m_lines.at(i + 1)->noteData());
        } else {
            m_lines.at(i)->setNoteData({});
        }
    }
    return changedPositions;
}

Column::PositionList Column::insertNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    Column::PositionList changedPositions;
    const auto newIndex = position.line;
    const auto iter = m_lines.insert(m_lines.begin() + static_cast<long>(newIndex), m_lines.back());
    (*iter)->setNoteData(noteData);
    m_lines.pop_back();
    for (size_t i = 0; i < m_lines.size(); i++) {
        if (i >= newIndex) {
            m_lines.at(i)->setIndex(i);
            auto changedPosition = position;
            changedPosition.line = i;
            changedPositions.push_back(changedPosition);
        }
    }
    return changedPositions;
}

Column::PositionList Column::transposeColumn(const Position & position, int semitones)
{
    Column::PositionList changedPositions;
    for (size_t i = 0; i < m_lines.size(); i++) {
        if (m_lines.at(i)->noteData()->type() == NoteData::Type::NoteOn) {
            m_lines.at(i)->noteData()->transpose(semitones);
            auto changedPosition = position;
            changedPosition.line = i;
            changedPositions.push_back(changedPosition);
        }
    }
    return changedPositions;
}

Column::EventList Column::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    EventList eventList;
    size_t tick = startTick;
    for (size_t i = 0; i < m_virtualLineCount; i++) {
        if (auto && line = m_lines.at(i); line->noteData()->type() != NoteData::Type::None) {
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

    for (size_t i = 0; i < m_virtualLineCount; i++) {
        if (auto && line = m_lines.at(i); line) {
            line->serializeToXml(writer);
        }
    }

    writer.writeEndElement(); // Lines
    writer.writeEndElement(); // Column
}

} // namespace noteahead
