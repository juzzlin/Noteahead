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

#include "automation_service_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../domain/interpolator.hpp"
#include "../../domain/midi_cc_automation.hpp"

#include <QSignalSpy>

namespace noteahead {

void MidiCcAutomationsModelTest::test_addMidiCcAutomation_shouldAddAutomation()
{
    AutomationService automationService;
    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
    const auto comment = "MIDI CC Automation Test";

    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment);

    QCOMPARE(lineDataChangedSpy.count(), line1 - line0 + 1);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).size(), 1);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).at(0).controller(), controller);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).at(0).id(), 1);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line0, line0);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line1, line1);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value0, value0);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value1, value1);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line0).at(0).comment(), comment);

    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line1).size(), 1);
    QCOMPARE(automationService.midiCcAutomationsByLine(pattern, track, column, line1 + 1).size(), 0);
    QCOMPARE(automationService.midiCcAutomationsByColumn(pattern, track, column).size(), 1);
    QCOMPARE(automationService.midiCcAutomationsByColumn(pattern, track, column + 1).size(), 0);
    QCOMPARE(automationService.midiCcAutomationsByTrack(pattern, track).size(), 1);
    QCOMPARE(automationService.midiCcAutomationsByTrack(pattern, track).size(), 1);
    QCOMPARE(automationService.midiCcAutomationsByPattern(pattern).size(), 1);
    QCOMPARE(automationService.midiCcAutomationsByPattern(pattern + 1).size(), 0);
    QCOMPARE(automationService.midiCcAutomations().size(), 1);

    QVERIFY(automationService.hasAutomations(pattern, track, column, line0));
    QVERIFY(automationService.hasAutomations(pattern, track, column, line1));
    QVERIFY(automationService.hasAutomations(pattern, track, column, (line0 + line1) / 2));
}

void MidiCcAutomationsModelTest::test_automationWeight_shouldCalculateCorrectWeight()
{
    AutomationService automationService;
    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 16;
    quint8 value0 = 0;
    quint8 value1 = 127;

    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {});

    QCOMPARE(automationService.automationWeight(pattern, track, column, line0), 0);
    QCOMPARE(automationService.automationWeight(pattern, track, column, line1), 1);
}

void MidiCcAutomationsModelTest::test_renderToEventsByLine_shouldRenderToEvents()
{
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;

    AutomationService automationService;
    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {});
            }
        }
    }

    const auto tick = 666;
    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, line0, tick).at(0)->type(), Event::Type::MidiCcData);
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, line0, tick).at(0)->tick(), tick);
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, line0, tick).at(0)->midiCcData()->track(), track);
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, line0, tick).at(0)->midiCcData()->column(), column);
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, line0, tick).at(0)->midiCcData()->controller(), controller);
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, line0, tick).at(0)->midiCcData()->value(), value0);
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, (line0 + line1) / 2, tick).at(0)->midiCcData()->value(), (value0 + value1) / 2);
                QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, line1, tick).at(0)->midiCcData()->value(), value1);
            }
        }
    }
}

void MidiCcAutomationsModelTest::test_renderToEventsByColumn_shouldRenderToEvents()
{
    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;

    AutomationService automationService;
    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {});

    const auto tick = 666;
    const auto ticksPerLine = 24;
    const auto events = automationService.renderToEventsByColumn(pattern, track, column, tick, ticksPerLine);
    QCOMPARE(automationService.renderToEventsByColumn(pattern, track, column, tick, tick).size(), line1 - line0 + 1);
    Interpolator interpolator {
        static_cast<size_t>(line0),
        static_cast<size_t>(line1),
        static_cast<double>(value0),
        static_cast<double>(value1)
    };
    for (size_t line = line0; line <= line1; line++) {
        const auto i = line - line0;
        QCOMPARE(events.at(i)->type(), Event::Type::MidiCcData);
        QCOMPARE(events.at(i)->tick(), tick + line * ticksPerLine);
        QCOMPARE(events.at(i)->midiCcData()->controller(), controller);
        QCOMPARE(events.at(i)->midiCcData()->value(), static_cast<uint8_t>(interpolator.getValue(line)));
        QCOMPARE(events.at(i)->midiCcData()->track(), track);
        QCOMPARE(events.at(i)->midiCcData()->column(), column);
    }
}

void MidiCcAutomationsModelTest::test_renderToEventsByColumn_shouldPruneRepeatingEvents()
{
    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 128;
    quint8 value0 = 10;
    quint8 value1 = 20;

    AutomationService automationService;
    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {});

    const auto tick = 666;
    const auto ticksPerLine = 24;
    const auto events = automationService.renderToEventsByColumn(pattern, track, column, tick, ticksPerLine);
    QCOMPARE(automationService.renderToEventsByColumn(pattern, track, column, tick, tick).size(), 11);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiCcAutomationsModelTest)
