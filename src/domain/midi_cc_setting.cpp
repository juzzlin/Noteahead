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

#include "midi_cc_setting.hpp"

#include "../common/constants.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

MidiCcSetting::MidiCcSetting(uint8_t controller, uint8_t value)
  : m_controller { controller }
  , m_value { value }
{
}

MidiCcSetting::MidiCcSetting()
{
}

uint8_t MidiCcSetting::controller() const
{
    return m_controller;
}

void MidiCcSetting::setController(uint8_t controller)
{
    m_controller = controller;
}

uint8_t MidiCcSetting::value() const
{
    return m_value;
}

void MidiCcSetting::setValue(uint8_t value)
{
    m_value = value;
}

void MidiCcSetting::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyMidiCcSetting());
    writer.writeAttribute(Constants::xmlKeyController(), QString::number(m_controller));
    writer.writeAttribute(Constants::xmlKeyValue(), QString::number(m_value));
    writer.writeEndElement();
}

QString MidiCcSetting::toString() const
{
    return QString { "MidiCcSetting ( controller=%1, value=%2 )" }
      .arg(m_controller)
      .arg(m_value);
}

} // namespace noteahead
