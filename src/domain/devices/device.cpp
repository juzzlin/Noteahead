// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#include "device.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

size_t Device::id() const
{
    return m_id;
}

void Device::setId(size_t id)
{
    m_id = id;
}

void Device::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeParametersToXml(writer);
    writer.writeEndElement(); // Device
}

void Device::deserializeFromXml(QXmlStreamReader & reader)
{
    deserializeAttributesFromXml(reader);
    reader.readNext();
    deserializeParametersFromXml(reader);
}

void Device::serializeAttributesToXml(QXmlStreamWriter & writer) const
{
    writer.writeAttribute(Constants::NahdXml::xmlKeyId(), QString::number(m_id));
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyCategory(), QString::fromStdString(category()));
}

void Device::deserializeAttributesFromXml(QXmlStreamReader & reader)
{
    if (const auto id = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyId(), false); id.has_value()) {
        m_id = id.value();
    }
}

} // namespace noteahead
