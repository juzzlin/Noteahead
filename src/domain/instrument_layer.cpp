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

#include "instrument_layer.hpp"

#include "../common/constants.hpp"
#include "../common/utils.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

InstrumentLayer::InstrumentLayer() = default;

InstrumentLayer::InstrumentLayer(size_t id, AutomationLocation location, Parameters parameters, QString comment, bool enabled)
  : Automation { id, location, comment, enabled }
  , m_parameters { parameters }
{
}

InstrumentLayer::InstrumentLayer(size_t id, AutomationLocation location, Parameters parameters, QString comment)
  : InstrumentLayer { id, location, parameters, comment, true }
{
}

bool InstrumentLayer::operator==(const InstrumentLayer & other) const
{
    return id() == other.id() && //
      location() == other.location() && //
      comment() == other.comment() && //
      enabled() == other.enabled();
}

bool InstrumentLayer::operator!=(const InstrumentLayer & other) const
{
    return !(*this == other);
}

bool InstrumentLayer::operator<(const InstrumentLayer & other) const
{
    return id() < other.id();
}

QString InstrumentLayer::toString() const
{
    return QString("InstrumentLayer(id=%1, pattern=%2, track=%3, column=%4), enabled=%10")
      .arg(QString::number(id()),
           QString::number(location().pattern()),
           QString::number(location().track()),
           QString::number(location().column()),
           QString::number(enabled()));
}

InstrumentLayer::Parameters InstrumentLayer::parameters() const
{
    return m_parameters;
}

void InstrumentLayer::setParameters(const Parameters & parameters)
{
    m_parameters = parameters;
}

void InstrumentLayer::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyInstrumentLayer());
    writer.writeAttribute(Constants::NahdXml::xmlKeyComment(), comment());
    writer.writeAttribute(Constants::NahdXml::xmlKeyEnabled(), enabled() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());

    location().serializeToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTargetTrack(), QString::number(m_parameters.targetTrack));
    writer.writeAttribute(Constants::NahdXml::xmlKeyNote(), QString::number(m_parameters.note));
    writer.writeAttribute(Constants::NahdXml::xmlKeyFollowSourceNote(), m_parameters.followSourceNote ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeyVelocity(), QString::number(m_parameters.velocity));
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplyTargetVelocity(), m_parameters.applyTargetVelocity ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFollowSourceVelocity(), m_parameters.followSourceVelocity ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeEndElement(); // Parameters

    writer.writeEndElement(); // Automation
}

InstrumentLayer::InstrumentLayerU InstrumentLayer::deserializeFromXml(QXmlStreamReader & reader)
{
    const QString comment = reader.attributes().value(Constants::NahdXml::xmlKeyComment()).toString();
    const bool enabled = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyEnabled(), false).value_or(true);
    AutomationLocation::AutomationLocationU location = std::make_unique<AutomationLocation>();
    Parameters parameters;
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyInstrumentLayer()))) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (!reader.name().compare(Constants::NahdXml::xmlKeyLocation())) {
                location = AutomationLocation::deserializeFromXml(reader);
            } else if (!reader.name().compare(Constants::NahdXml::xmlKeyParameters())) {
                const auto attributes = reader.attributes();
                parameters.targetTrack = attributes.value(Constants::NahdXml::xmlKeyTargetTrack()).toUInt();
                parameters.note = static_cast<quint8>(attributes.value(Constants::NahdXml::xmlKeyNote()).toUInt());
                parameters.followSourceNote =
                  Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyFollowSourceNote(), false).value_or(false);
                parameters.velocity = static_cast<quint8>(attributes.value(Constants::NahdXml::xmlKeyVelocity()).toUInt());
                parameters.applyTargetVelocity =
                  Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyApplyTargetVelocity(), false).value_or(false);
                parameters.followSourceVelocity =
                  Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyFollowSourceVelocity(), false).value_or(false);
            }
        }
    }
    return std::make_unique<InstrumentLayer>(0, *location, parameters, comment, enabled);
}

} // namespace noteahead
