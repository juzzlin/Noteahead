// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#include "instrument.hpp"

#include "../common/constants.hpp"
#include "../common/utils.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "Instrument";

Instrument::Instrument(const QString & portName)
  : m_midiAddress { portName }
{
}

Instrument::Instrument(const MidiAddress & midiAddress)
  : m_midiAddress { midiAddress }
{
}

const MidiAddress & Instrument::midiAddress() const
{
    return m_midiAddress;
}

void Instrument::setMidiAddress(const MidiAddress & device)
{
    m_midiAddress = device;
}

const InstrumentSettings & Instrument::settings() const
{
    return m_settings;
}

void Instrument::setSettings(const InstrumentSettings & settings)
{
    m_settings = settings;
}

void Instrument::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyInstrument());

    m_midiAddress.serializeToXml(writer);

    m_settings.serializeToXml(writer);

    writer.writeEndElement(); // Instrument
}

Instrument::InstrumentU Instrument::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Instrument";

    // Read mandatory properties
    auto instrument = std::make_unique<Instrument>(*MidiAddress::deserializeFromXml(reader));
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyInstrument()))) {
        juzzlin::L(TAG).trace() << "Instrument: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyInstrumentSettings())) {
            if (const auto settings = InstrumentSettings::deserializeFromXml(reader); settings) {
                instrument->m_settings = *settings;
            }
        }
        reader.readNext();
    }

    return instrument;
}

QString Instrument::toString() const
{
    auto result = QString { "Instrument ( portName='%1', channel=%2, group=%3 " }.arg(m_midiAddress.portName()).arg(m_midiAddress.channel()).arg(m_midiAddress.group());
    result += m_settings.toString();
    result += " )";
    return result;
}

} // namespace noteahead
