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

#include "midi_address.hpp"

#include "../common/constants.hpp"
#include "../common/utils.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

MidiAddress::MidiAddress(QString portName)
  : m_portName { portName }
{
}

MidiAddress::MidiAddress(QString portName, uint8_t channel)
  : m_portName { portName }
  , m_channel { channel }
{
}

MidiAddress::MidiAddress(QString portName, uint8_t channel, uint8_t group)
  : m_portName { portName }
  , m_channel { channel }
  , m_group { group }
{
}

QString MidiAddress::portName() const
{
    return m_portName;
}

uint8_t MidiAddress::channel() const
{
    return m_channel;
}

void MidiAddress::setChannel(uint8_t channel)
{
    m_channel = channel;
}

uint8_t MidiAddress::group() const
{
    return m_group;
}

void MidiAddress::setGroup(uint8_t group)
{
    m_group = group;
}

void MidiAddress::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeAttribute(Constants::NahdXml::xmlKeyPortName(), m_portName);
    writer.writeAttribute(Constants::NahdXml::xmlKeyChannel(), QString::number(m_channel));
    writer.writeAttribute(Constants::NahdXml::xmlKeyGroup(), QString::number(m_group));
}

MidiAddress::MidiAddressU MidiAddress::deserializeFromXml(QXmlStreamReader & reader)
{
    const auto portName = *Utils::Xml::readStringAttribute(reader, Constants::NahdXml::xmlKeyPortName());
    const auto channel = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChannel(), false).value_or(0);
    const auto group = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChannel(), false).value_or(0);
    return std::make_unique<MidiAddress>(portName, channel, group);
}

} // namespace noteahead
