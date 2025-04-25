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
#include "../common/utils.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

MidiCcAutomation::MidiCcAutomation() = default;

MidiCcAutomation::MidiCcAutomation(size_t id, Location location, uint8_t controller, Interpolation interpolation, QString comment, bool enabled)
  : m_id { id }
  , m_controller { controller }
  , m_location { location }
  , m_interpolation { interpolation }
  , m_comment { comment }
  , m_enabled { enabled }
{
}

MidiCcAutomation::MidiCcAutomation(size_t id, Location location, uint8_t controller, Interpolation interpolation, QString comment)
  : MidiCcAutomation { id, location, controller, interpolation, comment, true }
{
}

bool MidiCcAutomation::operator==(const MidiCcAutomation & other) const
{
    return m_id == other.m_id && //
      m_controller == other.m_controller && //
      m_location == other.m_location && //
      m_interpolation == other.m_interpolation && //
      m_comment == other.m_comment && //
      m_enabled == other.m_enabled;
}

bool MidiCcAutomation::operator!=(const MidiCcAutomation & other) const
{
    return !(*this == other);
}

bool MidiCcAutomation::operator<(const MidiCcAutomation & other) const
{
    return m_id < other.m_id;
}

size_t MidiCcAutomation::id() const
{
    return m_id;
}

void MidiCcAutomation::setId(size_t id)
{
    m_id = id;
}

uint8_t MidiCcAutomation::controller() const
{
    return m_controller;
}

QString MidiCcAutomation::comment() const
{
    return m_comment;
}

void MidiCcAutomation::setComment(QString comment)
{
    m_comment = comment;
}

void MidiCcAutomation::setController(uint8_t controller)
{
    m_controller = controller;
}

const MidiCcAutomation::Location & MidiCcAutomation::location() const
{
    return m_location;
}

void MidiCcAutomation::setLocation(const Location & location)
{
    m_location = location;
}

const MidiCcAutomation::Interpolation & MidiCcAutomation::interpolation() const
{
    return m_interpolation;
}

void MidiCcAutomation::setInterpolation(const Interpolation & interpolation)
{
    m_interpolation = interpolation;
}

bool MidiCcAutomation::enabled() const
{
    return m_enabled;
}

void MidiCcAutomation::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

QString MidiCcAutomation::toString() const
{
    return QString("MidiCcAutomation(id=%1, controller=%2, pattern=%3, track=%4, column=%5, "
                   "line: %6 -> %7, value: %8 -> %9), enabled=%10")
      .arg(QString::number(m_id),
           QString::number(m_controller),
           QString::number(m_location.pattern),
           QString::number(m_location.track),
           QString::number(m_location.column),
           QString::number(m_interpolation.line0),
           QString::number(m_interpolation.line1),
           QString::number(m_interpolation.value0),
           QString::number(m_interpolation.value1),
           QString::number(m_enabled));
}

void MidiCcAutomation::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyMidiCcAutomation());
    writer.writeAttribute(Constants::xmlKeyController(), QString::number(m_controller));
    writer.writeAttribute(Constants::xmlKeyComment(), m_comment);
    writer.writeAttribute(Constants::xmlKeyEnabled(), m_enabled ? Constants::xmlValueTrue() : Constants::xmlValueFalse());

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
    const quint8 controller = static_cast<quint8>(reader.attributes().value(Constants::xmlKeyController()).toUInt());
    const QString comment = reader.attributes().value(Constants::xmlKeyComment()).toString();
    const bool enabled = Utils::Xml::readBoolAttribute(reader, Constants::xmlKeyEnabled(), false).value_or(true);
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
    return std::make_unique<MidiCcAutomation>(0, location, controller, interpolation, comment, enabled);
}

} // namespace noteahead
