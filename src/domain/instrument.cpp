// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "instrument.hpp"

#include "../common/constants.hpp"

#include <QXmlStreamWriter>

namespace cacophony {

Instrument::Instrument(QString portName)
  : portName { portName }
{
}

void Instrument::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyInstrument());

    // Mandatory properties
    writer.writeAttribute(Constants::xmlKeyPortName(), portName);
    writer.writeAttribute(Constants::xmlKeyChannel(), QString::number(channel));

    // Optional properties
    if (patch.has_value()) {
        writer.writeAttribute(Constants::xmlKeyPatchEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyPatch(), QString::number(*patch));
    } else {
        writer.writeAttribute(Constants::xmlKeyPatchEnabled(), "false");
    }

    if (bank.has_value()) {
        writer.writeAttribute(Constants::xmlKeyBankEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyBankLsb(), QString::number(bank->lsb));
        writer.writeAttribute(Constants::xmlKeyBankMsb(), QString::number(bank->msb));
        writer.writeAttribute(Constants::xmlKeyBankByteOrderSwapped(), bank->byteOrderSwapped ? "true" : "false");
    } else {
        writer.writeAttribute(Constants::xmlKeyBankEnabled(), "false");
    }

    writer.writeEndElement(); // Instrument
}

} // namespace cacophony
