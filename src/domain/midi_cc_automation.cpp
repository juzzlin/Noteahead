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

#include "midi_cc_automation.hpp"

#include "../common/constants.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

MidiCcAutomation::MidiCcAutomation() = default;

MidiCcAutomation::MidiCcAutomation(size_t id, Location location, uint8_t controller, Interpolation interpolation, QString comment)
  : m_id { id }
  , m_controller { controller }
  , m_location { location }
  , m_interpolation { interpolation }
  , m_comment { comment }
{
}

size_t MidiCcAutomation::id() const
{
    return m_id;
}

uint8_t MidiCcAutomation::controller() const
{
    return m_controller;
}

QString MidiCcAutomation::comment() const
{
    return m_comment;
}

void MidiCcAutomation::setController(uint8_t controller)
{
    m_controller = controller;
}

const MidiCcAutomation::Location & MidiCcAutomation::location() const
{
    return m_location;
}

const MidiCcAutomation::Interpolation & MidiCcAutomation::interpolation() const
{
    return m_interpolation;
}

QString MidiCcAutomation::toString() const
{
    return QString("MidiCcAutomation(id=%1, CC=%2, pattern=%3, track=%4, column=%5, "
                   "line: %6 -> %7, value: %8 -> %9)")
      .arg(QString::number(m_id),
           QString::number(m_controller),
           QString::number(m_location.pattern),
           QString::number(m_location.track),
           QString::number(m_location.column),
           QString::number(m_interpolation.line0),
           QString::number(m_interpolation.line1),
           QString::number(m_interpolation.value0),
           QString::number(m_interpolation.value1));
}

void MidiCcAutomation::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyMidiCcAutomation());
    writer.writeAttribute(Constants::xmlKeyController(), QString::number(m_controller));
    writer.writeAttribute(Constants::xmlKeyComment(), m_comment);

    writer.writeStartElement(Constants::xmlKeyLocation());
    writer.writeAttribute(Constants::xmlKeyPatternAttr(), QString::number(m_location.pattern));
    writer.writeAttribute(Constants::xmlKeyTrackAttr(), QString::number(m_location.track));
    writer.writeAttribute(Constants::xmlKeyColumnAttr(), QString::number(m_location.column));
    writer.writeEndElement(); // Location

    writer.writeStartElement(Constants::xmlKeyInterpolation());
    writer.writeAttribute(Constants::xmlKeyLine0(), QString::number(m_interpolation.line0));
    writer.writeAttribute(Constants::xmlKeyLine1(), QString::number(m_interpolation.line1));
    writer.writeAttribute(Constants::xmlKeyValue0(), QString::number(m_interpolation.value0));
    writer.writeAttribute(Constants::xmlKeyValue1(), QString::number(m_interpolation.value1));
    writer.writeEndElement(); // Interpolation

    writer.writeEndElement(); // Automation
}

MidiCcAutomation::MidiCcAutomationU MidiCcAutomation::deserializeFromXml(QXmlStreamReader & reader)
{
    quint64 id = reader.attributes().value(Constants::xmlKeyId()).toULongLong();
    quint8 controller = static_cast<quint8>(reader.attributes().value(Constants::xmlKeyController()).toUInt());
    QString comment = reader.attributes().value(Constants::xmlKeyComment()).toString();
    MidiCcAutomation::Location location {};
    MidiCcAutomation::Interpolation interpolation {};
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyMidiCcAutomation()))) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::xmlKeyLocation())) {
                const auto attributes = reader.attributes();
                location.pattern = attributes.value(Constants::xmlKeyPatternAttr()).toULongLong();
                location.track = attributes.value(Constants::xmlKeyTrackAttr()).toULongLong();
                location.column = attributes.value(Constants::xmlKeyColumnAttr()).toULongLong();
            } else if (!reader.name().compare(Constants::xmlKeyInterpolation())) {
                const auto attributes = reader.attributes();
                interpolation.line0 = attributes.value(Constants::xmlKeyLine0()).toULongLong();
                interpolation.line1 = attributes.value(Constants::xmlKeyLine1()).toULongLong();
                interpolation.value0 = static_cast<quint8>(attributes.value(Constants::xmlKeyValue0()).toUInt());
                interpolation.value1 = static_cast<quint8>(attributes.value(Constants::xmlKeyValue1()).toUInt());
            }
        }
    }
    return std::make_unique<MidiCcAutomation>(id, location, controller, interpolation, comment);
}

} // namespace noteahead
