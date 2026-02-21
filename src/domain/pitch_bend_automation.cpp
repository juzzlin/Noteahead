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

#include "pitch_bend_automation.hpp"

#include "../common/constants.hpp"
#include "../common/utils.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

PitchBendAutomation::PitchBendAutomation() = default;

PitchBendAutomation::PitchBendAutomation(size_t id, AutomationLocation location, InterpolationParameters interpolation, QString comment, bool enabled)
  : Automation { id, location, comment, enabled }
  , m_interpolation { interpolation }
{
}

PitchBendAutomation::PitchBendAutomation(size_t id, AutomationLocation location, InterpolationParameters interpolation, QString comment)
  : PitchBendAutomation { id, location, interpolation, comment, true }
{
}

bool PitchBendAutomation::operator==(const PitchBendAutomation & other) const
{
    return id() == other.id() && //
      location() == other.location() && //
      m_interpolation == other.m_interpolation && //
      comment() == other.comment() && //
      enabled() == other.enabled();
}

bool PitchBendAutomation::operator!=(const PitchBendAutomation & other) const
{
    return !(*this == other);
}

bool PitchBendAutomation::operator<(const PitchBendAutomation & other) const
{
    return id() < other.id();
}

const PitchBendAutomation::InterpolationParameters & PitchBendAutomation::interpolation() const
{
    return m_interpolation;
}

void PitchBendAutomation::setInterpolation(const InterpolationParameters & interpolation)
{
    m_interpolation = interpolation;
}

QString PitchBendAutomation::toString() const
{
    return QString { "PitchBendAutomation(id=%1, controller=%2, pattern=%3, track=%4, column=%5, "
                     "line: %6 -> %7, value: %8 -> %9), enabled=%10" }
      .arg(QString::number(id()),
           QString::number(location().pattern()),
           QString::number(location().track()),
           QString::number(location().column()),
           QString::number(m_interpolation.line0),
           QString::number(m_interpolation.line1),
           QString::number(m_interpolation.value0),
           QString::number(m_interpolation.value1),
           QString::number(enabled()));
}

void PitchBendAutomation::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyPitchBendAutomation());
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

PitchBendAutomation::PitchBendAutomationU PitchBendAutomation::deserializeFromXml(QXmlStreamReader & reader)
{
    const QString comment = reader.attributes().value(Constants::NahdXml::xmlKeyComment()).toString();
    const bool enabled = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyEnabled(), false).value_or(true);
    AutomationLocation::AutomationLocationU location = std::make_unique<AutomationLocation>();
    PitchBendAutomation::InterpolationParameters parameters {};
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPitchBendAutomation()))) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::NahdXml::xmlKeyLocation())) {
                location = AutomationLocation::deserializeFromXml(reader);
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyInterpolation())) {
                const auto attributes = reader.attributes();
                parameters.line0 = attributes.value(Constants::NahdXml::xmlKeyLine0()).toULongLong();
                parameters.line1 = attributes.value(Constants::NahdXml::xmlKeyLine1()).toULongLong();
                parameters.value0 = attributes.value(Constants::NahdXml::xmlKeyValue0()).toInt();
                parameters.value1 = attributes.value(Constants::NahdXml::xmlKeyValue1()).toInt();
            }
        }
    }
    return std::make_unique<PitchBendAutomation>(0, *location, parameters, comment, enabled);
}

} // namespace noteahead
