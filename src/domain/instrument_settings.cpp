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

#include "instrument_settings.hpp"

#include "../common/constants.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

size_t InstrumentSettings::track() const
{
    return m_track;
}

void InstrumentSettings::setTrack(size_t track)
{
    m_track = track;
}

void InstrumentSettings::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyInstrumentSettings());

    if (patch.has_value()) {
        writer.writeAttribute(Constants::xmlKeyPatch(), QString::number(*patch));
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
        writer.writeAttribute(Constants::xmlKeyCutoff(), QString::number(*cutoff));
    }

    if (pan.has_value()) {
        writer.writeAttribute(Constants::xmlKeyPan(), QString::number(*pan));
    }

    if (volume.has_value()) {
        writer.writeAttribute(Constants::xmlKeyVolume(), QString::number(*volume));
    }

    for (auto && midiCcSetting : midiCcSettings) {
        midiCcSetting.serializeToXml(writer);
    }

    writer.writeEndElement(); // Settings
}

QString InstrumentSettings::toString() const
{
    auto result = QString { "InstrumentSettings (" };

    result += patch ? QString { " patch=%1" }.arg(*patch) : ", patch=None";

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
