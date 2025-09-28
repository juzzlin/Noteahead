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

    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote1Offset(), QString::number(chordAutomationSettings.note1.offset));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote1Velocity(), QString::number(chordAutomationSettings.note1.velocity));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote2Offset(), QString::number(chordAutomationSettings.note2.offset));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote2Velocity(), QString::number(chordAutomationSettings.note2.velocity));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote3Offset(), QString::number(chordAutomationSettings.note3.offset));
    writer.writeAttribute(Constants::NahdXml::xmlKeyChordNote3Velocity(), QString::number(chordAutomationSettings.note3.velocity));

    writer.writeEndElement();
}

ColumnSettings::ColumnSettingsU ColumnSettings::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).trace() << "Reading ColumnSettings";

    auto settings = std::make_unique<ColumnSettings>();

    settings->chordAutomationSettings.note1.offset = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote1Offset(), false).value_or(0);
    settings->chordAutomationSettings.note1.velocity = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote1Velocity(), false).value_or(100);
    settings->chordAutomationSettings.note2.offset = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote2Offset(), false).value_or(0);
    settings->chordAutomationSettings.note2.velocity = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote2Velocity(), false).value_or(100);
    settings->chordAutomationSettings.note3.offset = Utils::Xml::readIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote3Offset(), false).value_or(0);
    settings->chordAutomationSettings.note3.velocity = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyChordNote3Velocity(), false).value_or(100);

    return settings;
}

QString ColumnSettings::toString() const
{
    return QStringLiteral("ColumnSettings(chordAutomation: note1(offset=%1, velocity=%2), note2(offset=%3, velocity=%4), note3(offset=%5, velocity=%6))")
        .arg(chordAutomationSettings.note1.offset)
        .arg(chordAutomationSettings.note1.velocity)
        .arg(chordAutomationSettings.note2.offset)
        .arg(chordAutomationSettings.note2.velocity)
        .arg(chordAutomationSettings.note3.offset)
        .arg(chordAutomationSettings.note3.velocity);
}

} // namespace noteahead
