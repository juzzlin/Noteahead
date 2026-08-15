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

#include "../../application/position.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../midi/midi_cc_data.hpp"
#include "column.hpp"
#include "column_settings.hpp"
#include "event.hpp"
#include "instrument.hpp"
#include "note_data.hpp"
#include "track.hpp"

#include <algorithm>
#include <ranges>

namespace noteahead {

static const auto TAG = "Track";

Track::Track(size_t index, std::string name, size_t length, size_t columnCount)
  : MixerUnit { index, name }
{
    initialize(length, columnCount);
}

Track::Track(size_t index, std::string name, size_t length, const ColumnIndexList & columnIndices)
  : MixerUnit { index, name }
{
    resetColumnOrder(columnIndices, length);
}

void Track::initialize(size_t length, size_t columnCount)
{
    ColumnIndexList columnIndices;
    for (size_t column = 0; column < columnCount; column++) {
        columnIndices.push_back(column);
    }
    resetColumnOrder(columnIndices, length);
}

void Track::resetColumnOrder(const ColumnIndexList & columnIndices, size_t length)
{
    m_columnOrder.clear();
    for (auto && columnIndex : columnIndices) {
        m_columnOrder.push_back(std::make_shared<Column>(columnIndex, length));
    }
}

Track::ColumnS Track::columnByIndex(size_t columnIndex) const
{
    if (const auto it = std::ranges::find_if(m_columnOrder, [columnIndex](auto && column) { return column->index() == columnIndex; }); it != m_columnOrder.end()) {
        return *it;
    } else {
        return nullptr;
    }
}

Track::ColumnS Track::columnByIndexThrow(size_t columnIndex) const
{
    if (const auto column = columnByIndex(columnIndex); column) {
        return column;
    } else {
        juzzlin::L(TAG).error() << "Invalid column index: " << columnIndex;
        throw std::runtime_error("Invalid column index: " + std::to_string(columnIndex));
    }
}

size_t Track::nextFreeColumnIndex() const
{
    auto indices = columnIndices();
    std::ranges::copy(deletedColumnIndices(), std::back_inserter(indices));
    std::ranges::sort(indices);
    size_t expectedIndex = 0;
    for (auto && index : indices) {
        if (index != expectedIndex) {
            return expectedIndex;
        }
        expectedIndex++;
    }
    return expectedIndex;
}

bool Track::hasColumn(size_t columnIndex) const
{
    return columnByIndex(columnIndex) != nullptr;
}

Track::ColumnIndexList Track::columnIndices() const
{
    ColumnIndexList indices;
    std::ranges::transform(m_columnOrder, std::back_inserter(indices), [](auto && column) { return column->index(); });
    return indices;
}

Track::ColumnIndexList Track::deletedColumnIndices() const
{
    ColumnIndexList indices;
    std::ranges::transform(m_deletedColumns, std::back_inserter(indices), [](auto && column) { return column->index(); });
    return indices;
}

std::optional<size_t> Track::columnPositionByIndex(size_t columnIndex) const
{
    if (const auto it = std::ranges::find_if(m_columnOrder, [columnIndex](auto && column) { return column->index() == columnIndex; }); it != m_columnOrder.end()) {
        return static_cast<size_t>(std::distance(m_columnOrder.begin(), it));
    } else {
        return {};
    }
}

std::optional<size_t> Track::columnIndexByPosition(size_t columnPosition) const
{
    return columnPosition < m_columnOrder.size() ? std::optional<size_t> { m_columnOrder.at(columnPosition)->index() } : std::optional<size_t> {};
}

Track::InstrumentS Track::instrument() const
{
    return m_instrument;
}

void Track::setInstrument(InstrumentS instrument)
{
    m_instrument = instrument;
}

Track::ColumnSettingsS Track::columnSettings(size_t columnIndex) const
{
    return columnByIndexThrow(columnIndex)->settings();
}

void Track::setColumnSettings(size_t columnIndex, ColumnSettingsS settings)
{
    columnByIndexThrow(columnIndex)->setSettings(settings);
}

Track::InstrumentSettingsS Track::instrumentSettingsAtPosition(const Position & position) const
{
    return columnByIndexThrow(position.column)->instrumentSettings(position);
}

void Track::setInstrumentSettingsAtPosition(const Position & position, InstrumentSettingsS instrumentSettings)
{
    if (instrumentSettings) {
        instrumentSettings->setTrack(index());
    }
    columnByIndexThrow(position.column)->setInstrumentSettings(position, instrumentSettings);
}

Track::ColumnS Track::createOrRestoreColumn()
{
    if (!m_deletedColumns.empty()) {
        const auto restored = m_deletedColumns.back();
        m_deletedColumns.pop_back();
        juzzlin::L(TAG).debug() << "Restored column with index " << restored->index();
        return restored;
    } else {
        const auto newIndex = nextFreeColumnIndex();
        juzzlin::L(TAG).debug() << "Created column with index " << newIndex;
        return std::make_shared<Column>(newIndex, lineCount());
    }
}

void Track::addColumn()
{
    m_columnOrder.push_back(createOrRestoreColumn());
    juzzlin::L(TAG).debug() << "Added column with index " << m_columnOrder.back()->index();
}

bool Track::addColumnToLeftOf(size_t columnIndex)
{
    if (const auto columnPosition = columnPositionByIndex(columnIndex); columnPosition.has_value()) {
        const auto column = createOrRestoreColumn();
        m_columnOrder.insert(m_columnOrder.begin() + static_cast<long>(*columnPosition), column);
        juzzlin::L(TAG).debug() << "Added column with index " << column->index() << " to the left of position " << *columnPosition;
        return true;
    } else {
        juzzlin::L(TAG).error() << "Invalid column index: " << columnIndex;
        return false;
    }
}

bool Track::addColumnToRightOf(size_t columnIndex)
{
    if (const auto columnPosition = columnPositionByIndex(columnIndex); columnPosition.has_value()) {
        const auto column = createOrRestoreColumn();
        m_columnOrder.insert(m_columnOrder.begin() + static_cast<long>(*columnPosition) + 1, column);
        juzzlin::L(TAG).debug() << "Added column with index " << column->index() << " to the right of position " << *columnPosition;
        return true;
    } else {
        juzzlin::L(TAG).error() << "Invalid column index: " << columnIndex;
        return false;
    }
}

bool Track::deleteColumn()
{
    return !m_columnOrder.empty() && deleteColumn(m_columnOrder.back()->index());
}

bool Track::deleteColumn(size_t columnIndex)
{
    if (m_columnOrder.size() < 2) {
        return false;
    }

    if (const auto columnPosition = columnPositionByIndex(columnIndex); columnPosition.has_value()) {
        juzzlin::L(TAG).debug() << "Deleting column with index " << columnIndex << " at position " << *columnPosition;
        m_deletedColumns.push_back(m_columnOrder.at(*columnPosition));
        m_columnOrder.erase(m_columnOrder.begin() + static_cast<long>(*columnPosition));
        return true;
    } else {
        juzzlin::L(TAG).error() << "Invalid column index: " << columnIndex;
        return false;
    }
}

bool Track::moveColumnLeft(size_t columnIndex)
{
    if (m_columnOrder.size() < 2) {
        return false;
    }
    if (const auto columnPosition = columnPositionByIndex(columnIndex); columnPosition.has_value()) {
        if (*columnPosition) {
            std::swap(m_columnOrder.at(*columnPosition), m_columnOrder.at(*columnPosition - 1));
        } else {
            // Off the left end: wrap around to the right end, the others keeping their order
            std::rotate(m_columnOrder.begin(), m_columnOrder.begin() + 1, m_columnOrder.end());
        }
        return true;
    }
    return false;
}

bool Track::moveColumnRight(size_t columnIndex)
{
    if (m_columnOrder.size() < 2) {
        return false;
    }
    if (const auto columnPosition = columnPositionByIndex(columnIndex); columnPosition.has_value()) {
        if (*columnPosition + 1 < m_columnOrder.size()) {
            std::swap(m_columnOrder.at(*columnPosition), m_columnOrder.at(*columnPosition + 1));
        } else {
            // Off the right end: wrap around to the left end
            std::rotate(m_columnOrder.rbegin(), m_columnOrder.rbegin() + 1, m_columnOrder.rend());
        }
        return true;
    }
    return false;
}

void Track::setColumn(ColumnS column)
{
    if (const auto columnPosition = columnPositionByIndex(column->index()); columnPosition.has_value()) {
        m_columnOrder.at(*columnPosition) = column;
    } else if (const auto it = std::ranges::find_if(m_deletedColumns, [&column](auto && deleted) { return deleted->index() == column->index(); }); it != m_deletedColumns.end()) {
        *it = column;
    } else {
        // A column the order does not know about was deleted before the project was saved
        m_deletedColumns.push_back(column);
    }
}

std::string Track::columnName(size_t columnIndex) const
{
    return columnByIndexThrow(columnIndex)->name();
}

void Track::setColumnName(size_t columnIndex, std::string name)
{
    columnByIndexThrow(columnIndex)->setName(name);
}

std::optional<size_t> Track::columnByName(std::string_view name) const
{
    const auto column = std::ranges::find_if(m_columnOrder, [=](auto && column) { return column->name() == name; });
    return column != m_columnOrder.end() ? (*column)->index() : std::optional<size_t> {};
}

size_t Track::columnCount() const
{
    return m_columnOrder.size();
}

size_t Track::lineCount() const
{
    return m_columnOrder.at(0)->lineCount();
}

void Track::setLineCount(size_t lineCount)
{
    // The deleted columns are kept in sync so that restoring one gives a column of the right length
    for (auto && column : m_columnOrder) {
        column->setLineCount(lineCount);
    }
    for (auto && column : m_deletedColumns) {
        column->setLineCount(lineCount);
    }
}

Track::LineList Track::lines(const Position & position) const
{
    return columnByIndexThrow(position.column)->lines();
}

bool Track::hasData() const
{
    return std::ranges::any_of(m_columnOrder, [](auto && column) {
        return column->hasData();
    });
}

bool Track::hasData(size_t column) const
{
    const auto columnObject = columnByIndex(column);
    return columnObject && columnObject->hasData();
}

bool Track::hasPosition(const Position & position) const
{
    if (position.track == index()) {
        if (const auto column = columnByIndex(position.column); column) {
            return column->hasPosition(position);
        }
    }
    return false;
}

Position Track::nextNoteDataOnSameColumn(const Position & position) const
{
    return columnByIndexThrow(position.column)->nextNoteDataOnSameColumn(position);
}

Position Track::prevNoteDataOnSameColumn(const Position & position) const
{
    return columnByIndexThrow(position.column)->prevNoteDataOnSameColumn(position);
}

Track::NoteDataS Track::noteDataAtPosition(const Position & position) const
{
    return columnByIndexThrow(position.column)->noteDataAtPosition(position);
}

void Track::setNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Set note data at position: " << noteData.toString() << " @ " << position.toString();
    auto newNoteData = noteData;
    newNoteData.setTrack(index());
    columnByIndexThrow(position.column)->setNoteDataAtPosition(newNoteData, position);
}

Track::PositionList Track::deleteNoteDataAtPosition(const Position & position)
{
    juzzlin::L(TAG).debug() << "Delete note data at position: " << position.toString();
    return columnByIndexThrow(position.column)->deleteNoteDataAtPosition(position);
}

Track::PositionList Track::insertNoteDataAtPosition(const NoteData & noteData, const Position & position)
{
    juzzlin::L(TAG).debug() << "Insert note data at position: " << noteData.toString() << " @ " << position.toString();
    return columnByIndexThrow(position.column)->insertNoteDataAtPosition(noteData, position);
}

NoteChangeList Track::transposeTrack(const Position & position, int semitones) const
{
    NoteChangeList changes;
    for (auto && column : m_columnOrder) {
        auto columnPosition = position;
        columnPosition.column = column->index();
        auto columnChanges = column->transposeColumn(columnPosition, semitones);
        changes.insert(changes.end(), columnChanges.begin(), columnChanges.end());
    }
    return changes;
}

NoteChangeList Track::transposeColumn(const Position & position, int semitones) const
{
    return columnByIndexThrow(position.column)->transposeColumn(position, semitones);
}

Track::EventList Track::renderPanToEvents(size_t startTick, size_t ticksPerLine) const
{
    Track::EventList eventList;
    size_t tick = startTick;
    for (size_t line = 0; line < lineCount(); line++) {
        size_t sum = 0;
        size_t count = 0;
        for (auto && column : m_columnOrder) {
            if (const auto pan = column->pan(line); pan.has_value()) {
                sum += *pan;
                count++;
            }
        }
        if (count) {
            const auto averagePan = static_cast<uint8_t>((sum + count / 2) / count); // Rounded to nearest
            eventList.push_back(std::make_shared<Event>(tick, MidiCcData { index(), 0, Constants::panMidiCcController(), averagePan }));
        }
        tick += ticksPerLine;
    }
    return eventList;
}

Track::EventList Track::renderToEvents(size_t startTick, size_t ticksPerLine) const
{
    // The pan events come first on purpose: PlayerWorker dispatches the events of a tick in
    // insertion order, and a note starting on the same tick has to see the new pan, not the old one.
    Track::EventList eventList = renderPanToEvents(startTick, ticksPerLine);
    for (auto && column : m_columnOrder) {
        const auto columnList = column->renderToEvents(startTick, ticksPerLine);
        std::ranges::copy(columnList, std::back_inserter(eventList));
    }
    return eventList;
}

void Track::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement("Track");

    writer.writeAttribute("index", QString::number(index()));
    writer.writeAttribute("name", QString::fromStdString(name()));
    writer.writeAttribute("lineCount", QString::number(lineCount()));
    writer.writeAttribute("columnCount", QString::number(columnCount()));

    // Only a track with deleted or reordered columns needs its indices spelled out, so that a
    // project that has neither serializes exactly as it always did
    const auto columnIndices = this->columnIndices();
    if (!std::ranges::equal(columnIndices, std::views::iota(size_t { 0 }, columnCount()))) {
        QStringList indexStrings;
        std::ranges::transform(columnIndices, std::back_inserter(indexStrings), [](auto && columnIndex) { return QString::number(columnIndex); });
        writer.writeAttribute(Constants::NahdXml::xmlKeyColumnIndices(), indexStrings.join(","));
    }

    if (m_instrument) {
        m_instrument->serializeToXml(writer);
    }

    // The deleted columns are written out too: a soft delete must not lose data at save time. They
    // are the ones whose index the columnIndices attribute does not list.
    auto columnsToWrite = m_columnOrder;
    std::ranges::copy(m_deletedColumns, std::back_inserter(columnsToWrite));
    if (std::ranges::any_of(columnsToWrite, [](auto && column) { return column->hasData(); })) {
        writer.writeStartElement("Columns");
        for (auto && column : columnsToWrite) {
            if (column && column->hasData()) {
                column->serializeToXml(writer);
            }
        }
        writer.writeEndElement(); // Columns
    }

    writer.writeEndElement(); // Track
}

Track::TrackU Track::deserializeFromXml(ProjectReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Track started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyIndex());
    const auto name = *Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyName());
    const auto columnCount = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyColumnCount());
    const auto lineCount = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyLineCount());
    auto track = std::make_unique<Track>(index, name.toStdString(), lineCount, columnCount);
    // Absent on a project that never had a column deleted or moved, where the order is 0..columnCount-1
    if (const auto columnIndices = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyColumnIndices(), false); columnIndices.has_value()) {
        ColumnIndexList indices;
        for (auto && indexString : columnIndices->split(",", Qt::SkipEmptyParts)) {
            indices.push_back(indexString.toULongLong());
        }
        track->resetColumnOrder(indices, lineCount);
    }
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyTrack()))) {
        juzzlin::L(TAG).trace() << "Track: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyColumns())) {
            deserializeColumns(reader, *track);
        } else if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyInstrument())) {
            if (auto instrument = Instrument::deserializeFromXml(reader); instrument) {
                track->setInstrument(std::move(instrument));
            }
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Track ended";
    return track;
}

void Track::deserializeColumns(ProjectReader & reader, Track & track)
{
    juzzlin::L(TAG).trace() << "Reading Columns started";
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyColumns()))) {
        juzzlin::L(TAG).trace() << "Columns: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyColumn())) {
            track.setColumn(Column::deserializeFromXml(reader, track.index()));
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Columns ended";
}

} // namespace noteahead
