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
#include "../common/utils.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "instrument_settings.hpp"

#include <QXmlStreamReader>
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
    juzzlin::L(TAG).trace() << "Set note data " << noteData.toString();
    *m_noteData = noteData;
}

Line::NoteDataS Line::noteData() const
{
    return m_noteData;
}

Line::LineEventOpt Line::lineEvent() const
{
    return m_lineEvent;
}

void Line::setLineEvent(LineEventOpt lineEvent)
{
    if (lineEvent) {
        if (lineEvent->instrumentSettings()) {
            juzzlin::L(TAG).debug() << "Set instrument settings " << lineEvent->instrumentSettings()->toString().toStdString();
        }
    } else {
        juzzlin::L(TAG).debug() << "Reset LineEvent";
    }
    m_lineEvent = lineEvent;
}

bool Line::hasData() const
{
    return (m_noteData && m_noteData->type() != NoteData::Type::None) || m_lineEvent;
}

void Line::serializeToXml(QXmlStreamWriter & writer) const
{
    if (hasData()) {
        writer.writeStartElement(Constants::xmlKeyLine());
        writer.writeAttribute(Constants::xmlKeyIndex(), QString::number(m_index));
        if (m_noteData->type() != NoteData::Type::None) {
            m_noteData->serializeToXml(writer);
        }
        if (m_lineEvent) {
            m_lineEvent->serializeToXml(writer);
        }
        writer.writeEndElement(); // Line
    }
}

Line::LineU Line::deserializeFromXml(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex)
{
    juzzlin::L(TAG).trace() << "Reading Line started";
    const auto index = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyIndex());
    auto line = std::make_unique<Line>(index);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyLine()))) {
        juzzlin::L(TAG).trace() << "Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyNoteData())) {
            line->setNoteData(*NoteData::deserializeFromXml(reader, trackIndex, columnIndex));
        } else if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyLineEvent())) {
            line->setLineEvent(*LineEvent::deserializeFromXml(reader, trackIndex, columnIndex));
        }
        reader.readNext();
    }
    juzzlin::L(TAG).trace() << "Reading Line ended";
    return line;
}

} // namespace noteahead
