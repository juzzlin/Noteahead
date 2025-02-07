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

#include <QXmlStreamWriter>

namespace noteahead {

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

    if (cutoff.has_value()) {
        writer.writeAttribute(Constants::xmlKeyCutoffEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyCutoff(), QString::number(*cutoff));
    } else {
        writer.writeAttribute(Constants::xmlKeyCutoffEnabled(), "false");
    }

    if (pan.has_value()) {
        writer.writeAttribute(Constants::xmlKeyPanEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyPan(), QString::number(*pan));
    } else {
        writer.writeAttribute(Constants::xmlKeyPanEnabled(), "false");
    }

    if (volume.has_value()) {
        writer.writeAttribute(Constants::xmlKeyVolumeEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyVolume(), QString::number(*volume));
    } else {
        writer.writeAttribute(Constants::xmlKeyVolumeEnabled(), "false");
    }

    writer.writeEndElement(); // Instrument
}

QString Instrument::toString() const
{
    auto result = QString { "Instrument ( portName='%1', channel=%2" }.arg(portName).arg(channel);

    result += patch ? QString { ", patch=%1" }.arg(*patch) : ", patch=None";

    if (bank) {
        result += QString { ", bank={lsb=%1, msb=%2, byteOrderSwapped=%3}" }
                    .arg(bank->lsb)
                    .arg(bank->msb)
                    .arg(bank->byteOrderSwapped ? "true" : "false");
    } else {
        result += ", bank=None";
    }

    result += cutoff ? QString { ", cutoff=%1" }.arg(*cutoff) : ", cutoff=None";
    result += pan ? QString { ", pan=%1" }.arg(*pan) : ", pan=None";
    result += volume ? QString { ", volume=%1" }.arg(*volume) : ", volume=None";

    result += " )";
    return result;
}

} // namespace noteahead
