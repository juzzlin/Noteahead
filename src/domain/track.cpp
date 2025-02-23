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

#include "track.hpp"

#include "../application/position.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/instrument.hpp"
#include "../domain/note_data.hpp"
#include "../domain/track.hpp"
#include "column.hpp"

#include <QXmlStreamWriter>

#include <algorithm>

namespace noteahead {

static const auto TAG = "Track";

Track::Track(size_t index, std::string name, size_t length, size_t columnCount)
  : MixerUnit { index, name }
{
    initialize(length, columnCount);
}

void Track::initialize(size_t length, size_t columnCount)
{
    m_columns.clear();
    for (size_t column = 0; column < columnCount; column++) {
        m_columns.push_back(std::make_shared<Column>(column, length));
    }
    m_virtualColumnCount = m_columns.size();
}

Track::InstrumentS Track::instrument() const
{
    return m_instrument;
}

void Track::setInstrument(InstrumentS instrument)
{
    m_instrument = instrument;
}

Track::InstrumentSettingsS Track::instrumentSettings(const Position & position) const
{
    return m_columns.at(position.column)->instrumentSettings(position);
}

void Track::setInstrumentSettings(const Position & position, InstrumentSettingsS instrumentSettings)
{
    if (instrumentSettings) {
        instrumentSettings->setTrack(index());
    }
    m_columns.at(position.column)->setInstrumentSettings(position, instrumentSettings);
}

void Track::addColumn()
{
    if (m_virtualColumnCount < m_columns.size()) {
        m_virtualColumnCount++;
    } else {
        const auto length = m_columns.back()->lineCount();
        m_columns.push_back(std::make_shared<Column>(m_columns.size(), length));
        m_virtualColumnCount = m_columns.size();
    }
}

bool Track::deleteColumn()
{
    if (m_virtualColumnCount > 1) {
        m_virtualColumnCount--;
        return true;
    }
    return false;
}

void Track::setColumn(ColumnS column)
{
    m_columns.at(column->index()) = column;
}

std::string Track::columnName(size_t columnIndex) const
{
    return m_columns.at(columnIndex)->name();
}

void Track::setColumnName(size_t columnIndex, std::string name)
{
    m_columns.at(columnIndex)->setName(name);
}

size_t Track::columnCount() const
{
    return m_virtualColumnCount;
}

size_t Track::lineCount() const
{
    return m_columns.at(0)->lineCount();
}

void Track::setLineCount(size_t lineCount)
{
    for (auto && column : m_columns) {
        column->setLineCount(lineCount);
    }
}

bool Track::hasData() const
{
    return std::ranges::find_if(m_columns, [this](auto && column) {
               return column->index() < m_virtualColumnCount && column->hasData();
           })
      != m_columns.end();
}

bool Track::hasData(size_t column) const
{
    return m_columns.at(column)->hasData();
}

bool Track::hasPosition(const Position & position) const
{
    if (position.track == index() && position.column < m_columns.size()) {
        return m_columns.at(position.column)->hasPosition(position);
    }
    return false;
}

Position Track::nextNoteDataOnSameColumn(const Position & position) const
{
    return m_columns.at(position.column)->nextNoteDataOnSameColumn(position);
}

Position Track::prevNoteDataOnSameColumn(const Position & position) const
{
    return m_columns.at(position.column)->prevNoteDataOnSameColumn(position);
}

Track::NoteDataS Track::noteDataAtPosition(const Position & position) const
{
    return m_columns.at(position.column)->noteDataAtPosition(position);
}

void Track::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    auto newNoteData = noteData;
    newNoteData.setTrack(index());
    m_columns.at(position.column)->setNoteDataAtPosition(newNoteData, position);
}

Track::PositionList Track::deleteNoteDataAtPosition(const Position & position)
{
    juzzlin::L(TAG).debug() << "Delete note data at position: " << position.toString();
    return m_columns.at(position.column)->deleteNoteDataAtPosition(position);
}

Track::PositionList Track::insertNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Insert note data at position: " << noteData.toString() << " @ " << position.toString();
    return m_columns.at(position.column)->insertNoteDataAtPosition(noteData, position);
}

Track::PositionList Track::transposeTrack(const Position & position, int semitones) const
{
    Track::PositionList changedPositions;
    for (auto && column : m_columns) {
        auto columnPosition = position;
        columnPosition.column = column->index();
        std::ranges::copy(column->transposeColumn(columnPosition, semitones), std::back_inserter(changedPositions));
    }
    return changedPositions;
}

Track::PositionList Track::transposeColumn(const Position & position, int semitones) const
{
    return m_columns.at(position.column)->transposeColumn(position, semitones);
}

Track::EventList Track::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    Track::EventList eventList;
    for (size_t columnIndex = 0; columnIndex < m_virtualColumnCount; columnIndex++) {
        const auto column = m_columns.at(columnIndex);
        const auto columnList = column->renderToEvents(startTick, ticksPerLine);
        std::copy(columnList.begin(), columnList.end(), std::back_inserter(eventList));
    }
    return eventList;
}

void Track::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement("Track");

    writer.writeAttribute("index", QString::number(index()));
    writer.writeAttribute("name", QString::fromStdString(name()));
    writer.writeAttribute("lineCount", QString::number(lineCount()));
    writer.writeAttribute("columnCount", QString::number(columnCount()));

    if (m_instrument) {
        m_instrument->serializeToXml(writer);
    }

    if (hasData()) {
        writer.writeStartElement("Columns");
        for (size_t columnIndex = 0; columnIndex < m_virtualColumnCount; columnIndex++) {
            if (const auto column = m_columns.at(columnIndex); column && column->hasData()) {
                column->serializeToXml(writer);
            }
        }
        writer.writeEndElement(); // Columns
    }

    writer.writeEndElement(); // Track
}

} // namespace noteahead
