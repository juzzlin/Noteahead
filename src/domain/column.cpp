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
#include "../common/utils.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/event.hpp"
#include "../domain/note_data.hpp"
#include "line.hpp"
#include "line_event.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

namespace noteahead {

static const auto TAG = "Column";

Column::Column(size_t index, size_t length)
  : MixerUnit { index, "" }
{
    initialize(length);
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
    return !name().empty() || std::ranges::any_of(m_lines, [](auto && line) {
        return line->hasData();
    });
}

bool Column::hasPosition(const Position & position) const
{
    return position.column == index() && position.line < m_virtualLineCount;
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
    auto prevNoteDataPosition = position;
    for (size_t line = 0; line < position.line; line++) {
        if (const auto noteData = m_lines.at(line)->noteData(); noteData->type() != NoteData::Type::None) {
            prevNoteDataPosition.line = line;
        }
    }
    return prevNoteDataPosition;
}

Column::NoteDataS Column::noteDataAtPosition(const Position & position) const
{
    return m_lines.at(static_cast<size_t>(position.line))->noteData();
}

void Column::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    auto newNoteData = noteData;
    newNoteData.setColumn(index());
    m_lines.at(static_cast<size_t>(position.line))->setNoteData(newNoteData);
}

Column::LineList Column::lines() const
{
    return {
        m_lines.begin(),
        m_lines.begin() + static_cast<long>(m_virtualLineCount)
    };
}

Column::PositionList Column::addChangedPosition(const Column::PositionList & changedPositions, const Position & position, size_t line) const
{
    Column::PositionList newChangedPositions = changedPositions;
    auto changedPosition = position;
    changedPosition.line = line;
    if (hasPosition(changedPosition)) {
        newChangedPositions.push_back(changedPosition);
    }
    return newChangedPositions;
}

Column::PositionList Column::deleteNoteDataAtPosition(const Position & position)
{
    juzzlin::L(TAG).debug() << "Delete note data at position: " << position.toString();
    Column::PositionList changedPositions;
    for (size_t i = position.line; i < m_lines.size(); i++) {
        if (i + 1 < m_lines.size()) {
            m_lines.at(i)->setNoteData(*m_lines.at(i + 1)->noteData());
        } else {
            m_lines.at(i)->setNoteData({});
        }
        changedPositions = addChangedPosition(changedPositions, position, i);
    }
    return changedPositions;
}

Column::PositionList Column::insertNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Insert note data at position: " << noteData.toString() << " @ " << position.toString();
    Column::PositionList changedPositions;
    const size_t newIndex = position.line;
    if (newIndex >= m_lines.size()) {
        return changedPositions;
    }
    for (size_t i = m_lines.size() - 1; i > newIndex; i--) {
        m_lines.at(i)->setNoteData(*m_lines.at(i - 1)->noteData());
        changedPositions = addChangedPosition(changedPositions, position, i);
    }
    m_lines.at(newIndex)->setNoteData(noteData);
    changedPositions = addChangedPosition(changedPositions, position, newIndex);
    return changedPositions;
}

Column::PositionList Column::transposeColumn(const Position & position, int semitones)
{
    Column::PositionList changedPositions;
    for (size_t i = 0; i < m_lines.size(); i++) {
        if (m_lines.at(i)->noteData()->type() == NoteData::Type::NoteOn) {
            m_lines.at(i)->noteData()->transpose(semitones);
            changedPositions = addChangedPosition(changedPositions, position, i);
        }
    }
    return changedPositions;
}

Column::EventList Column::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    EventList eventList;
    size_t tick = startTick;
    for (size_t i = 0; i < m_virtualLineCount; i++) {
        if (auto && line = m_lines.at(i)) {
            if (line->lineEvent()) {
                if (line->lineEvent()->instrumentSettings()) {
                    eventList.push_back(std::make_shared<Event>(tick, line->lineEvent()->instrumentSettings()));
                }
            }
            if (const auto noteData = line->noteData(); noteData->type() != NoteData::Type::None) {
                eventList.push_back(std::make_shared<Event>(tick + noteData->delay(), *noteData));
            }
        }
        tick += ticksPerLine;
    }
    return eventList;
}

Column::InstrumentSettingsS Column::instrumentSettings(const Position & position) const
{
    const auto lineEvent = m_lines.at(position.line)->lineEvent();
    return lineEvent ? lineEvent->instrumentSettings() : nullptr;
}

void Column::setInstrumentSettings(const Position & position, InstrumentSettingsS instrumentSettings)
{
    LineEvent lineEvent { position.track, position.column };
    lineEvent.setInstrumentSettings(instrumentSettings);
    m_lines.at(position.line)->setLineEvent(lineEvent);
}

void Column::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyColumn());
    writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(index()));
    writer.writeAttribute(Constants::xmlKeyName(), QString::fromStdString(name()));
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

Column::ColumnU Column::deserializeFromXml(QXmlStreamReader & reader, size_t trackIndex)
{
    juzzlin::L(TAG).trace() << "Reading Column started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyIndex());
    const auto lineCount = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyLineCount());
    auto column = std::make_unique<Column>(index, lineCount);
    if (const auto name = Utils::Xml::readStringAttribute(reader, Constants::xmlKeyName(), false); name.has_value()) {
        juzzlin::L(TAG).trace() << "Setting column index=" << index << " name to '" << name->toStdString() << "'";
        column->setName(name->toStdString());
    }
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyColumn()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLines())) {
            deserializeLines(reader, trackIndex, *column);
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Column ended";
    return column;
}

void Column::deserializeLines(QXmlStreamReader & reader, size_t trackIndex, Column & column)
{
    juzzlin::L(TAG).trace() << "Reading Lines started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLines()))) {
        juzzlin::L(TAG).trace() << "Deserializing Line: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLine())) {
            column.addOrReplaceLine(Line::deserializeFromXml(reader, trackIndex, column.index()));
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Lines ended";
}

} // namespace noteahead
