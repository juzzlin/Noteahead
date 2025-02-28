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

Instrument::Instrument(QString portName)
  : device { portName }
{
}

void Instrument::serializeDevice(QXmlStreamWriter & writer) const
{
    writer.writeAttribute(Constants::xmlKeyPortName(), device.portName);
    writer.writeAttribute(Constants::xmlKeyChannel(), QString::number(device.channel));
}

void Instrument::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyInstrument());

    serializeDevice(writer);

    settings.serializeToXml(writer);

    writer.writeEndElement(); // Instrument
}

Instrument::InstrumentU Instrument::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading Instrument";

    // Read mandatory properties
    const auto portName = *Utils::Xml::readStringAttribute(reader, Constants::xmlKeyPortName());
    const auto channel = *Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyChannel());
    auto instrument = std::make_unique<Instrument>(portName);
    instrument->device.channel = static_cast<uint8_t>(channel);
    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyInstrument()))) {
        juzzlin::L(TAG).trace() << "Instrument: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyInstrumentSettings())) {
            if (const auto settings = InstrumentSettings::deserializeFromXml(reader); settings) {
                instrument->settings = *settings;
            }
        }
        reader.readNext();
    }

    return instrument;
}

QString Instrument::toString() const
{
    auto result = QString { "Instrument ( portName='%1', channel=%2 " }.arg(device.portName).arg(device.channel);
    result += settings.toString();
    result += " )";
    return result;
}

} // namespace noteahead
