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
  : device { portName }
{
}

void Instrument::serializeDevice(QXmlStreamWriter & writer) const
{
    writer.writeAttribute(Constants::xmlKeyPortName(), device.portName);
    writer.writeAttribute(Constants::xmlKeyChannel(), QString::number(device.channel));
}

void Instrument::serializeSettings(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeySettings());

    if (settings.patch.has_value()) {
        writer.writeAttribute(Constants::xmlKeyPatchEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyPatch(), QString::number(*settings.patch));
    } else {
        writer.writeAttribute(Constants::xmlKeyPatchEnabled(), "false");
    }

    if (settings.bank.has_value()) {
        writer.writeAttribute(Constants::xmlKeyBankEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyBankLsb(), QString::number(settings.bank->lsb));
        writer.writeAttribute(Constants::xmlKeyBankMsb(), QString::number(settings.bank->msb));
        writer.writeAttribute(Constants::xmlKeyBankByteOrderSwapped(), settings.bank->byteOrderSwapped ? "true" : "false");
    } else {
        writer.writeAttribute(Constants::xmlKeyBankEnabled(), "false");
    }

    if (settings.cutoff.has_value()) {
        writer.writeAttribute(Constants::xmlKeyCutoffEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyCutoff(), QString::number(*settings.cutoff));
    } else {
        writer.writeAttribute(Constants::xmlKeyCutoffEnabled(), "false");
    }

    if (settings.pan.has_value()) {
        writer.writeAttribute(Constants::xmlKeyPanEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyPan(), QString::number(*settings.pan));
    } else {
        writer.writeAttribute(Constants::xmlKeyPanEnabled(), "false");
    }

    if (settings.volume.has_value()) {
        writer.writeAttribute(Constants::xmlKeyVolumeEnabled(), "true");
        writer.writeAttribute(Constants::xmlKeyVolume(), QString::number(*settings.volume));
    } else {
        writer.writeAttribute(Constants::xmlKeyVolumeEnabled(), "false");
    }

    writer.writeEndElement(); // Settings
}

void Instrument::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyInstrument());

    serializeDevice(writer);

    serializeSettings(writer);

    writer.writeEndElement(); // Instrument
}

QString Instrument::toString() const
{
    auto result = QString { "Instrument ( portName='%1', channel=%2" }.arg(device.portName).arg(device.channel);

    result += settings.patch ? QString { ", patch=%1" }.arg(*settings.patch) : ", patch=None";

    if (settings.bank) {
        result += QString { ", bank={lsb=%1, msb=%2, byteOrderSwapped=%3}" }
                    .arg(settings.bank->lsb)
                    .arg(settings.bank->msb)
                    .arg(settings.bank->byteOrderSwapped ? "true" : "false");
    } else {
        result += ", bank=None";
    }

    result += settings.cutoff ? QString { ", cutoff=%1" }.arg(*settings.cutoff) : ", cutoff=None";
    result += settings.pan ? QString { ", pan=%1" }.arg(*settings.pan) : ", pan=None";
    result += settings.volume ? QString { ", volume=%1" }.arg(*settings.volume) : ", volume=None";

    result += " )";
    return result;
}

} // namespace noteahead
