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

#include "column_settings.hpp"

#include "../common/constants.hpp"
#include "../common/utils.hpp"
#include "../common/utils.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QStringBuilder>

namespace noteahead {

static const auto TAG = "ColumnSettings";

ColumnSettings::ColumnSettings() = default;

void ColumnSettings::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyColumnSettings());

    writer.writeAttribute(Constants::NahdXml::xmlKeyDelay(), QString::number(delay.count()));

    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote1Offset(), QString::number(chordAutomationSettings.note1.offset));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote1Velocity(), QString::number(chordAutomationSettings.note1.velocity));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote1Delay(), QString::number(chordAutomationSettings.note1.delay));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote2Offset(), QString::number(chordAutomationSettings.note2.offset));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote2Velocity(), QString::number(chordAutomationSettings.note2.velocity));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote2Delay(), QString::number(chordAutomationSettings.note2.delay));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote3Offset(), QString::number(chordAutomationSettings.note3.offset));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote3Velocity(), QString::number(chordAutomationSettings.note3.velocity));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote3Delay(), QString::number(chordAutomationSettings.note3.delay));

    writer.writeAttribute(Constants::NahdXml::xmlKeyArpeggiatorEnabled(), chordAutomationSettings.arpeggiator.enabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeyArpeggiatorPattern(), QString::number(static_cast<int>(chordAutomationSettings.arpeggiator.pattern)));
    writer.writeAttribute(Constants::NahdXml::xmlKeyArpeggiatorEventsPerBeat(), QString::number(chordAutomationSettings.arpeggiator.eventsPerBeat));

    writer.writeEndElement(); // ColumnSettings
}

ColumnSettings::ColumnSettingsU ColumnSettings::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading ColumnSettings";

    auto settings = std::make_unique<ColumnSettings>();

    settings->delay = std::chrono::milliseconds { Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyDelay(), false).value_or(0) };

    settings->chordAutomationSettings.note1.offset = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote1Offset(), false).value_or(0);
    settings->chordAutomationSettings.note1.velocity = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote1Velocity(), false).value_or(100);
    settings->chordAutomationSettings.note1.delay = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote1Delay(), false).value_or(0);
    settings->chordAutomationSettings.note2.offset = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote2Offset(), false).value_or(0);
    settings->chordAutomationSettings.note2.velocity = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote2Velocity(), false).value_or(100);
    settings->chordAutomationSettings.note2.delay = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote2Delay(), false).value_or(0);
    settings->chordAutomationSettings.note3.offset = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote3Offset(), false).value_or(0);
    settings->chordAutomationSettings.note3.velocity = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote3Velocity(), false).value_or(100);
    settings->chordAutomationSettings.note3.delay = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote3Delay(), false).value_or(0);

    settings->chordAutomationSettings.arpeggiator.enabled = Utils::Xml::readBoolAttribute(reader, Constants::NahdXml::xmlKeyArpeggiatorEnabled(), false).value_or(false);
    settings->chordAutomationSettings.arpeggiator.pattern = static_cast<Arpeggiator::Pattern>(Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyArpeggiatorPattern(), false).value_or(0));
    settings->chordAutomationSettings.arpeggiator.eventsPerBeat = static_cast<uint8_t>(Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyArpeggiatorEventsPerBeat(), false).value_or(4));

    return settings;
}

QString ColumnSettings::toString() const
{
    return QStringLiteral("ColumnSettings(delay=%1, chordAutomation: note1(offset=%2, velocity=%3, delay=%4), note2(offset=%5, velocity=%6, delay=%7), note3(offset=%8, velocity=%9, delay=%10))")
        .arg(delay.count())
        .arg(chordAutomationSettings.note1.offset)
        .arg(chordAutomationSettings.note1.velocity)
        .arg(chordAutomationSettings.note1.delay)
        .arg(chordAutomationSettings.note2.offset)
        .arg(chordAutomationSettings.note2.velocity)
        .arg(chordAutomationSettings.note2.delay)
        .arg(chordAutomationSettings.note3.offset)
        .arg(chordAutomationSettings.note3.velocity)
        .arg(chordAutomationSettings.note3.delay);
}

} // namespace noteahead
