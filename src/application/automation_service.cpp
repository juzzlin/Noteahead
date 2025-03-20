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

#include "automation_service.hpp"

#include "position.hpp"

#include <algorithm>

#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "AutomationService";

AutomationService::AutomationService()
{
}

void AutomationService::addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment)
{
    const auto index = m_midiCcAutomations.size() + 1;
    MidiCcAutomation::Location location = { pattern, track, column };
    MidiCcAutomation::Interpolation interpolation = { line0, line1, value0, value1 };
    const auto automation = MidiCcAutomation { index, location, controller, interpolation, comment };
    m_midiCcAutomations.push_back(automation);
    notifyChangedLines(pattern, track, column, line0, line1);
    juzzlin::L(TAG).info() << "MIDI CC Automation added: " << automation.toString().toStdString();
}

bool AutomationService::hasAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const
{
    const auto match = std::ranges::find_if(m_midiCcAutomations, [&](auto && automation) {
        auto && location = automation.location();
        auto && interpolation = automation.interpolation();
        return location.pattern == pattern && location.track == track && location.column == column && line >= interpolation.line0 && line <= interpolation.line1;
    });
    return match != m_midiCcAutomations.end();
}

void AutomationService::notifyChangedLines(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1)
{
    for (auto line = line0; line <= line1; line++) {
        Position changedPosition;
        changedPosition.pattern = pattern;
        changedPosition.track = track;
        changedPosition.column = column;
        changedPosition.line = line;
        emit lineDataChanged(changedPosition);
    }
}

void AutomationService::deserializeFromXml(QXmlStreamReader & reader)
{
    m_midiCcAutomations.clear();

    while (!(reader.isEndElement() && !reader.name().compare(Constants::xmlKeyAutomation()))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::xmlKeyMidiCcAutomation())) {
            m_midiCcAutomations.push_back(*MidiCcAutomation::deserializeFromXml(reader));
        }
        reader.readNext();
    }
}

void AutomationService::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyAutomation());

    for (const auto & automation : m_midiCcAutomations) {
        automation.serializeToXml(writer);
    }

    writer.writeEndElement(); // AutomationService
}

} // namespace noteahead
