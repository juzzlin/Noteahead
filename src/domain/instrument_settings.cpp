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
#include "../common/utils.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "InstrumentSettings";

void InstrumentSettings::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyInstrumentSettings());

    if (patch.has_value()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyPatch(), QString::number(*patch));
    }

    if (bank.has_value()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyBankEnabled(), Constants::NahdXml::xmlValueTrue());
        writer.writeAttribute(Constants::NahdXml::xmlKeyBankLsb(), QString::number(bank->lsb));
        writer.writeAttribute(Constants::NahdXml::xmlKeyBankMsb(), QString::number(bank->msb));
        writer.writeAttribute(Constants::NahdXml::xmlKeyBankByteOrderSwapped(), bank->byteOrderSwapped ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    } else {
        writer.writeAttribute(Constants::NahdXml::xmlKeyBankEnabled(), Constants::NahdXml::xmlValueFalse());
    }

    if (predefinedMidiCcSettings.cutoff.has_value()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyCutoff(), QString::number(*predefinedMidiCcSettings.cutoff));
    }

    if (predefinedMidiCcSettings.pan.has_value()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyPan(), QString::number(*predefinedMidiCcSettings.pan));
    }

    if (predefinedMidiCcSettings.volume.has_value()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeyVolume(), QString::number(*predefinedMidiCcSettings.volume));
    }

    if (sendMidiClock.has_value()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeySendMidiClock(), sendMidiClock.value() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    }
    if (sendTransport.has_value()) {
        writer.writeAttribute(Constants::NahdXml::xmlKeySendTransport(), sendTransport.value() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    }

    writer.writeAttribute(Constants::NahdXml::xmlKeyDelay(), QString::number(delay.count()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTranspose(), QString::number(transpose));

    for (auto && midiCcSetting : midiCcSettings) {
        midiCcSetting.serializeToXml(writer);
    }

    writer.writeEndElement(); // Settings
}

InstrumentSettings::InstrumentSettingsU InstrumentSettings::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading InstrumentSettings";

    auto settings = std::make_unique<InstrumentSettings>();

    settings->patch = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyPatch(), false);
    if (const auto bankEnabled = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyBankEnabled(), false); bankEnabled.has_value() && *bankEnabled) {
        const auto bankLsb = static_cast<uint8_t>(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyBankLsb()));
        const auto bankMsb = static_cast<uint8_t>(*Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyBankMsb()));
        const auto bankByteOrderSwapped = *Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyBankByteOrderSwapped());
        settings->bank = { bankLsb, bankMsb, bankByteOrderSwapped };
    }
    settings->predefinedMidiCcSettings.cutoff = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyCutoff(), false);
    settings->predefinedMidiCcSettings.pan = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyPan(), false);
    settings->predefinedMidiCcSettings.volume = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyVolume(), false);
    settings->sendMidiClock = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeySendMidiClock(), false);
    settings->sendTransport = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeySendTransport(), false);
    settings->delay = std::chrono::milliseconds { Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyDelay(), false).value_or(0) };
    settings->transpose = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyTranspose(), false).value_or(0);

    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyInstrumentSettings()))) {
        juzzlin::L(TAG).trace() << "InstrumentSettings: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyMidiCcSetting())) {
            settings->midiCcSettings.push_back(*MidiCcSetting::deserializeFromXml(reader));
        }
        reader.readNext();
    }

    return settings;
}

QString InstrumentSettings::toString() const
{
    auto result = QString { "InstrumentSettings (" };

    result += patch ? QString { " patch=%1" }.arg(*patch) : ", patch=None";

    if (bank) {
        result += QString { ", bank={lsb=%1, msb=%2, byteOrderSwapped=%3}" }
                    .arg(bank->lsb)
                    .arg(bank->msb)
                    .arg(bank->byteOrderSwapped ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    } else {
        result += ", bank=None";
    }

    result += predefinedMidiCcSettings.cutoff.has_value() ? QString { ", cutoff=%1" }.arg(*predefinedMidiCcSettings.cutoff) : ", cutoff=None";
    result += predefinedMidiCcSettings.pan.has_value() ? QString { ", pan=%1" }.arg(*predefinedMidiCcSettings.pan) : ", pan=None";
    result += predefinedMidiCcSettings.volume.has_value() ? QString { ", volume=%1" }.arg(*predefinedMidiCcSettings.volume) : ", volume=None";
    result += sendMidiClock.has_value() ? QString { ", sendMidiClock=%1" }.arg(sendMidiClock.value() ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse()) : ", sendMidiClock=None";
    result += QString { ", delay=%1" }.arg(delay.count());
    result += QString { ", transpose=%1" }.arg(transpose);

    for (auto && midiCcSetting : midiCcSettings) {
        result += " ";
        result += midiCcSetting.toString();
    }

    result += " )";
    return result;
}

} // namespace noteahead
