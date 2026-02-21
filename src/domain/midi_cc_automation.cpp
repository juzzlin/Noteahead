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

MidiCcAutomation::MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, ModulationParameters modulation, QString comment, bool enabled)
  : Automation { id, location, comment, enabled }
  , m_controller { controller }
  , m_interpolation { interpolation }
  , m_modulation { modulation }
{
}

MidiCcAutomation::MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, QString comment, bool enabled)
  : MidiCcAutomation { id, location, controller, interpolation, {}, comment, enabled }
{
}

MidiCcAutomation::MidiCcAutomation(size_t id, AutomationLocation location, uint8_t controller, InterpolationParameters interpolation, QString comment)
  : MidiCcAutomation { id, location, controller, interpolation, {}, comment, true }
{
}

bool MidiCcAutomation::operator==(const MidiCcAutomation & other) const
{
    return id() == other.id() && //
      m_controller == other.m_controller && //
      m_eventsPerBeat == other.m_eventsPerBeat && //
      m_lineOffset == other.m_lineOffset && //
      location() == other.location() && //
      m_interpolation == other.m_interpolation && //
      m_modulation == other.m_modulation && //
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

uint8_t MidiCcAutomation::eventsPerBeat() const
{
    return m_eventsPerBeat;
}

void MidiCcAutomation::setEventsPerBeat(uint8_t eventsPerBeat)
{
    m_eventsPerBeat = eventsPerBeat;
}

uint8_t MidiCcAutomation::lineOffset() const
{
    return m_lineOffset;
}

void MidiCcAutomation::setLineOffset(uint8_t lineOffset)
{
    m_lineOffset = lineOffset;
}

const MidiCcAutomation::InterpolationParameters & MidiCcAutomation::interpolation() const
{
    return m_interpolation;
}

void MidiCcAutomation::setInterpolation(const InterpolationParameters & interpolation)
{
    m_interpolation = interpolation;
}

const MidiCcAutomation::ModulationParameters & MidiCcAutomation::modulation() const
{
    return m_modulation;
}

void MidiCcAutomation::setModulation(const ModulationParameters & modulation)
{
    m_modulation = modulation;
}

QString MidiCcAutomation::toString() const
{
    return QString { "MidiCcAutomation(id=%1, controller=%2, pattern=%3, track=%4, column=%5, "
                     "line: %6 -> %7, value: %8 -> %9, eventsPerBeat=%10, lineOffset=%11), enabled=%12" }
      .arg(QString::number(id()),
           QString::number(m_controller),
           QString::number(location().pattern()),
           QString::number(location().track()),
           QString::number(location().column()),
           QString::number(m_interpolation.line0),
           QString::number(m_interpolation.line1),
           QString::number(m_interpolation.value0),
           QString::number(m_interpolation.value1),
           QString::number(m_eventsPerBeat),
           QString::number(m_lineOffset),
           QString::number(enabled()));
}

void MidiCcAutomation::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyMidiCcAutomation());
    writer.writeAttribute(Constants::NahdXml::xmlKeyController(), QString::number(m_controller));
    writer.writeAttribute(Constants::NahdXml::xmlKeyEventsPerBeat(), QString::number(m_eventsPerBeat));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLineOffset(), QString::number(m_lineOffset));
    writer.writeAttribute(Constants::NahdXml::xmlKeyComment(), comment());
    writer.writeAttribute(Constants::NahdXml::xmlKeyEnabled(), enabled() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());

    location().serializeToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInterpolation());
    writer.writeAttribute(Constants::NahdXml::xmlKeyLine0(), QString::number(m_interpolation.line0));
    writer.writeAttribute(Constants::NahdXml::xmlKeyLine1(), QString::number(m_interpolation.line1));
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue0(), QString::number(m_interpolation.value0));
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue1(), QString::number(m_interpolation.value1));
    writer.writeEndElement(); // Interpolation

    if (m_modulation.cycles > 0.f || m_modulation.amplitude > 0.f || m_modulation.offset != 0.f) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyModulation());
        writer.writeAttribute(Constants::NahdXml::xmlKeyCycles(), QString::number(static_cast<int>(m_modulation.cycles)));
        writer.writeAttribute(Constants::NahdXml::xmlKeyAmplitude(), QString::number(static_cast<int>(m_modulation.amplitude)));
        writer.writeAttribute(Constants::NahdXml::xmlKeyOffset(), QString::number(static_cast<int>(m_modulation.offset)));
        writer.writeAttribute(Constants::NahdXml::xmlKeyInverted(), m_modulation.inverted ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
        writer.writeEndElement(); // Modulation
    }

    writer.writeEndElement(); // Automation
}

MidiCcAutomation::MidiCcAutomationU MidiCcAutomation::deserializeFromXml(QXmlStreamReader & reader)
{
    const quint8 controller = static_cast<quint8>(reader.attributes().value(Constants::NahdXml::xmlKeyController()).toUInt());
    const quint8 eventsPerBeat = static_cast<quint8>(reader.attributes().value(Constants::NahdXml::xmlKeyEventsPerBeat()).toUInt());
    const quint8 lineOffset = static_cast<quint8>(reader.attributes().value(Constants::NahdXml::xmlKeyLineOffset()).toUInt());
    const QString comment = reader.attributes().value(Constants::NahdXml::xmlKeyComment()).toString();
    const bool enabled = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyEnabled(), false).value_or(true);
    AutomationLocation::AutomationLocationU location = std::make_unique<AutomationLocation>();
    MidiCcAutomation::InterpolationParameters interpolationParameters {};
    MidiCcAutomation::ModulationParameters modulationParameters {};

    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyMidiCcAutomation()))) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::NahdXml::xmlKeyLocation())) {
                location = AutomationLocation::deserializeFromXml(reader);
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyInterpolation())) {
                const auto attributes = reader.attributes();
                interpolationParameters.line0 = attributes.value(Constants::NahdXml::xmlKeyLine0()).toULongLong();
                interpolationParameters.line1 = attributes.value(Constants::NahdXml::xmlKeyLine1()).toULongLong();
                interpolationParameters.value0 = static_cast<quint8>(attributes.value(Constants::NahdXml::xmlKeyValue0()).toUInt());
                interpolationParameters.value1 = static_cast<quint8>(attributes.value(Constants::NahdXml::xmlKeyValue1()).toUInt());
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyModulation())) {
                const auto attributes = reader.attributes();
                modulationParameters.cycles = static_cast<float>(attributes.value(Constants::NahdXml::xmlKeyCycles()).toInt());
                modulationParameters.amplitude = static_cast<float>(attributes.value(Constants::NahdXml::xmlKeyAmplitude()).toInt());
                modulationParameters.offset = static_cast<float>(attributes.value(Constants::NahdXml::xmlKeyOffset()).toInt());
                modulationParameters.inverted = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyInverted(), false).value_or(false);
            }
        }
    }
    auto automation = std::make_unique<MidiCcAutomation>(0, *location, controller, interpolationParameters, modulationParameters, comment, enabled);
    automation->setEventsPerBeat(eventsPerBeat);
    automation->setLineOffset(lineOffset);
    return automation;
}

} // namespace noteahead
