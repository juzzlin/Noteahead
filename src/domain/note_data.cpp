// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "note_data.hpp"

#include "../common/constants.hpp"
#include "../common/utils.hpp"

#include <sstream>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

NoteData::NoteData(size_t track, size_t column)
  : EventData { track, column }
{
}

NoteData::NoteData() = default;

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

void NoteData::setAsNoteOff()
{
    m_type = Type::NoteOff;
    m_note = {};
}

void NoteData::transpose(int semitones)
{
    if (m_note.has_value() && ((semitones > 0 && *m_note + semitones <= 127) || (semitones < 0 && *m_note + semitones >= 0))) {
        m_note = *m_note + semitones;
    }
}

NoteData::Type NoteData::type() const
{
    return m_type;
}

std::optional<uint8_t> NoteData::note() const
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
       << "Type: " << (m_type == Type::NoteOn ? "NoteOn" : "NoteOff")
       << " Track: " << static_cast<int>(track()) << " Column: " << static_cast<int>(column())
       << " Note: " << (m_note.has_value() ? std::to_string(static_cast<int>(*m_note)) : "N/A")
       << " Velocity: " << static_cast<int>(m_velocity)
       << " Delay: " << static_cast<int>(m_delay.value_or(0)) << " ]";
    return ss.str();
}

uint8_t NoteData::delay() const
{
    return m_delay.value_or(0);
}

void NoteData::setDelay(uint8_t ticks)
{
    m_delay = ticks;
}

void NoteData::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyNoteData());

    if (m_type == Type::NoteOn) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyType(), Constants::NahdXml::xmlKeyNoteOn());
        writer.writeAttribute(Constants::NahdXml::xmlKeyNote(), QString::number(*m_note));
        writer.writeAttribute(Constants::NahdXml::xmlKeyVelocity(), QString::number(m_velocity));
        if (m_delay.has_value()) {
            writer.writeAttribute(Constants::NahdXml::xmlKeyDelay(), QString::number(*m_delay));
        }
    } else {
        writer.writeAttribute(Constants::NahdXml::xmlKeyType(), Constants::NahdXml::xmlKeyNoteOff());
    }

    writer.writeEndElement(); // NoteData
}

NoteData::NoteDataS NoteData::deserializeFromXml(QXmlStreamReader & reader, size_t trackIndex, size_t columnIndex)
{
    const auto typeString = Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyType());
    const auto type = typeString == Constants::NahdXml::xmlKeyNoteOn() ? NoteData::Type::NoteOn : NoteData::Type::NoteOff;
    auto noteData = std::make_unique<NoteData>(trackIndex, columnIndex);
    if (type == NoteData::Type::NoteOn) {
        const auto note = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyNote());
        const auto velocity = *Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyVelocity());
        noteData->setAsNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity));
        if (const auto delay = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyDelay(), false); delay.has_value()) {
            noteData->setDelay(static_cast<uint8_t>(*delay));
        }
    } else {
        noteData->setAsNoteOff();
    }

    return noteData;
}

} // namespace noteahead
