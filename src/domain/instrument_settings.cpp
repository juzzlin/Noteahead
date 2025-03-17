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
        writer.writeAttribute(Constants::xmlKeyBankEnabled(), Constants::xmlValueTrue());
        writer.writeAttribute(Constants::xmlKeyBankLsb(), QString::number(bank->lsb));
        writer.writeAttribute(Constants::xmlKeyBankMsb(), QString::number(bank->msb));
        writer.writeAttribute(Constants::xmlKeyBankByteOrderSwapped(), bank->byteOrderSwapped ? Constants::xmlValueTrue() : Constants::xmlValueFalse());
    } else {
        writer.writeAttribute(Constants::xmlKeyBankEnabled(), Constants::xmlValueFalse());
    }

    if (predefinedMidiCcSettings.cutoff.has_value()) {
        writer.writeAttribute(Constants::xmlKeyCutoff(), QString::number(*predefinedMidiCcSettings.cutoff));
    }

    if (predefinedMidiCcSettings.pan.has_value()) {
        writer.writeAttribute(Constants::xmlKeyPan(), QString::number(*predefinedMidiCcSettings.pan));
    }

    if (predefinedMidiCcSettings.volume.has_value()) {
        writer.writeAttribute(Constants::xmlKeyVolume(), QString::number(*predefinedMidiCcSettings.volume));
    }

    if (sendMidiClock.has_value()) {
        writer.writeAttribute(Constants::xmlKeySendMidiClock(), sendMidiClock.value() ? Constants::xmlValueTrue() : Constants::xmlValueFalse());
    }

    writer.writeAttribute(Constants::xmlKeyDelay(), QString::number(delay.count()));

    for (auto && midiCcSetting : midiCcSettings) {
        midiCcSetting.serializeToXml(writer);
    }

    writer.writeEndElement(); // Settings
}

std::unique_ptr<MidiCcSetting> deserializeMidiCcSetting(QXmlStreamReader & reader)
{
    const auto controller = static_cast<uint8_t>(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyController()));
    const auto value = static_cast<uint8_t>(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyValue()));
    return std::make_unique<MidiCcSetting>(controller, value);
}

InstrumentSettings::InstrumentSettingsU InstrumentSettings::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading InstrumentSettings";

    auto settings = std::make_unique<InstrumentSettings>();

    settings->patch = Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyPatch(), false);
    if (const auto bankEnabled = Utils::Xml::readBoolAttribute(reader, Constants::xmlKeyBankEnabled(), false); bankEnabled.has_value() && *bankEnabled) {
        const auto bankLsb = static_cast<uint8_t>(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyBankLsb()));
        const auto bankMsb = static_cast<uint8_t>(*Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyBankMsb()));
        const auto bankByteOrderSwapped = *Utils::Xml::readBoolAttribute(reader, Constants::xmlKeyBankByteOrderSwapped());
        settings->bank = { bankLsb, bankMsb, bankByteOrderSwapped };
    }
    settings->predefinedMidiCcSettings.cutoff = Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyCutoff(), false);
    settings->predefinedMidiCcSettings.pan = Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyPan(), false);
    settings->predefinedMidiCcSettings.volume = Utils::Xml::readUIntAttribute(reader, Constants::xmlKeyVolume(), false);
    settings->sendMidiClock = Utils::Xml::readBoolAttribute(reader, Constants::xmlKeySendMidiClock(), false);
    settings->delay = std::chrono::milliseconds { Utils::Xml::readIntAttribute(reader, Constants::xmlKeyDelay(), false).value_or(0) };

    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyInstrumentSettings()))) {
        juzzlin::L(TAG).trace() << "InstrumentSettings: Current element: " << reader.name().toString().toStdString();
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyMidiCcSetting())) {
            settings->midiCcSettings.push_back(*deserializeMidiCcSetting(reader));
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
                    .arg(bank->byteOrderSwapped ? Constants::xmlValueTrue() : Constants::xmlValueFalse());
    } else {
        result += ", bank=None";
    }

    result += predefinedMidiCcSettings.cutoff.has_value() ? QString { ", cutoff=%1" }.arg(*predefinedMidiCcSettings.cutoff) : ", cutoff=None";
    result += predefinedMidiCcSettings.pan.has_value() ? QString { ", pan=%1" }.arg(*predefinedMidiCcSettings.pan) : ", pan=None";
    result += predefinedMidiCcSettings.volume.has_value() ? QString { ", volume=%1" }.arg(*predefinedMidiCcSettings.volume) : ", volume=None";
    result += sendMidiClock.has_value() ? QString { ", sendMidiClock=%1" }.arg(sendMidiClock.value() ? Constants::xmlValueTrue() : Constants::xmlValueFalse()) : ", sendMidiClock=None";
    result += QString { ", delay=%1" }.arg(delay.count());

    for (auto && midiCcSetting : midiCcSettings) {
        result += " ";
        result += midiCcSetting.toString();
    }

    result += " )";
    return result;
}

} // namespace noteahead
