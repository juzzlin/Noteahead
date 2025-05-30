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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

MidiCcAutomation::MidiCcAutomation() = default;

MidiCcAutomation::MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, QString comment, bool enabled)
  : Automation { id, location, comment, enabled }
  , m_controller { controller }
  , m_interpolation { interpolation }
{
}

MidiCcAutomation::MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, QString comment)
  : MidiCcAutomation { id, location, controller, interpolation, comment, true }
{
}

bool MidiCcAutomation::operator==(const MidiCcAutomation & other) const
{
    return id() == other.id() && //
      m_controller == other.m_controller && //
      location() == other.location() && //
      m_interpolation == other.m_interpolation && //
      comment() == other.comment() && //
      enabled() == other.enabled();
}

bool MidiCcAutomation::operator!=(const MidiCcAutomation & other) const
{
    return !(*this == other);
}

bool MidiCcAutomation::operator<(const MidiCcAutomation & other) const
{
    return id() < other.id();
}

uint8_t MidiCcAutomation::controller() const
{
    return m_controller;
}

void MidiCcAutomation::setController(uint8_t controller)
{
    m_controller = controller;
}

const MidiCcAutomation::InterpolationParameters & MidiCcAutomation::interpolation() const
{
    return m_interpolation;
}

void MidiCcAutomation::setInterpolation(const InterpolationParameters & interpolation)
{
    m_interpolation = interpolation;
}

QString MidiCcAutomation::toString() const
{
    return QString("MidiCcAutomation(id=%1, controller=%2, pattern=%3, track=%4, column=%5, "
                   "line: %6 -> %7, value: %8 -> %9), enabled=%10")
      .arg(QString::number(id()),
           QString::number(m_controller),
           QString::number(location().pattern()),
           QString::number(location().track()),
           QString::number(location().column()),
           QString::number(m_interpolation.line0),
           QString::number(m_interpolation.line1),
           QString::number(m_interpolation.value0),
           QString::number(m_interpolation.value1),
           QString::number(enabled()));
}

void MidiCcAutomation::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyMidiCcAutomation());
    writer.writeAttribute(Constants::NahdXml::xmlKeyController(), QString::number(m_controller));
    writer.writeAttribute(Constants::NahdXml::xmlKeyComment(), comment());
    writer.writeAttribute(Constants::NahdXml::xmlKeyEnabled(), enabled() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());

    location().serializeToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInterpolation());
    writer.writeAttribute(Constants::NahdXml::xmlKeyLine0(), QString::number(m_interpolation.line0));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLine1(), QString::number(m_interpolation.line1));
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue0(), QString::number(m_interpolation.value0));
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue1(), QString::number(m_interpolation.value1));
    writer.writeEndElement(); // Interpolation

    writer.writeEndElement(); // Automation
}

MidiCcAutomation::MidiCcAutomationU MidiCcAutomation::deserializeFromXml(QXmlStreamReader & reader)
{
    const quint8 controller = static_cast<quint8>(reader.attributes().value(Constants::NahdXml::xmlKeyController()).toUInt());
    const QString comment = reader.attributes().value(Constants::NahdXml::xmlKeyComment()).toString();
    const bool enabled = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyEnabled(), false).value_or(true);
    AutomationLocation::AutomationLocationU location = std::make_unique<AutomationLocation>();
    MidiCcAutomation::InterpolationParameters parameters {};
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyMidiCcAutomation()))) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::NahdXml::xmlKeyLocation())) {
                location = AutomationLocation::deserializeFromXml(reader);
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyInterpolation())) {
                const auto attributes = reader.attributes();
                parameters.line0 = attributes.value(Constants::NahdXml::xmlKeyLine0()).toULongLong();
                parameters.line1 = attributes.value(Constants::NahdXml::xmlKeyLine1()).toULongLong();
                parameters.value0 = static_cast<quint8>(attributes.value(Constants::NahdXml::xmlKeyValue0()).toUInt());
                parameters.value1 = static_cast<quint8>(attributes.value(Constants::NahdXml::xmlKeyValue1()).toUInt());
            }
        }
    }
    return std::make_unique<MidiCcAutomation>(0, *location, controller, parameters, comment, enabled);
}

} // namespace noteahead
