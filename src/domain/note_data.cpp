// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "note_data.hpp"

#include <sstream>

#include <QXmlStreamWriter>

namespace cacophony {

NoteData::NoteData(uint32_t track, uint32_t column)
  : m_track { track }
  , m_column { column }
{
}

void NoteData::setAsNoteOn(uint8_t note, uint8_t velocity)
{
    m_type = Type::NoteOn;
    m_note = note;
    m_velocity = velocity;
}

void NoteData::setAsNoteOff(uint8_t note)
{
    m_type = Type::NoteOff;
    m_note = note;
}

NoteData::Type NoteData::type() const
{
    return m_type;
}

uint8_t NoteData::note() const
{
    return m_note;
}

uint8_t NoteData::velocity() const
{
    return m_velocity;
}

void NoteData::setVelocity(uint8_t velocity)
{
    m_velocity = velocity;
}

std::string NoteData::toString() const
{
    std::stringstream ss;
    ss << "[ "
       << "Type: " << static_cast<int>(m_type)
       << " Track: " << static_cast<int>(m_track) << " Column: " << static_cast<int>(m_column)
       << " Note: " << static_cast<int>(m_note) << " Velocity: " << static_cast<int>(m_velocity) << " ]";
    return ss.str();
}

uint32_t NoteData::track() const
{
    return m_track;
}

uint32_t NoteData::column() const
{
    return m_column;
}

void NoteData::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement("NoteData");

    if (m_type == Type::NoteOn) {
        writer.writeAttribute("type", "on");
        writer.writeAttribute("note", QString::number(m_note));
        writer.writeAttribute("velocity", QString::number(m_velocity));
    } else {
        writer.writeAttribute("type", "off");
        writer.writeAttribute("note", QString::number(m_note));
    }

    writer.writeEndElement(); // NoteData
}

} // namespace cacophony
