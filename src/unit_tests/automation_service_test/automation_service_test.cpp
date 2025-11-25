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

#include "../../application/position.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../domain/interpolator.hpp"
#include "../../domain/midi_cc_automation.hpp"

#include <QSignalSpy>

Q_DECLARE_METATYPE(noteahead::Position)

namespace noteahead {

void AutomationServiceTest::initTestCase()
{
    qRegisterMetaType<noteahead::Position>("Position");
}

void AutomationServiceTest::test_addMidiCcAutomation_shouldAddAutomation()
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

    auto id = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment);
    QCOMPARE(id, 1);
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

    id = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment);
    QCOMPARE(id, 2);
    id = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment);
    QCOMPARE(id, 3);
}

void AutomationServiceTest::test_deleteMidiCcAutomation_shouldDeleteAutomation()
{
    AutomationService automationService;

    automationService.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {});
    QVERIFY(!automationService.midiCcAutomations().empty());
    auto automation = automationService.midiCcAutomations().at(0);
    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    automationService.deleteMidiCcAutomation(automation);

    QVERIFY(automationService.midiCcAutomations().empty());
    QCOMPARE(lineDataChangedSpy.count(), automation.interpolation().line1 - automation.interpolation().line0 + 1);
}

void AutomationServiceTest::test_addPitchBendAutomation_shouldAddAutomation()
{
    AutomationService automationService;

    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 line0 = 4;
    quint8 line1 = 12;
    int value0 = -100;
    int value1 = +100;
    const auto comment = "Pitch Bend Automation Test";

    auto id = automationService.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, comment);
    QCOMPARE(id, 1);
    QCOMPARE(lineDataChangedSpy.count(), line1 - line0 + 1);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line0).size(), 1);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).id(), 1);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line0, line0);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line1, line1);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value0, value0);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value1, value1);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).comment(), comment);

    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line1).size(), 1);
    QCOMPARE(automationService.pitchBendAutomationsByLine(pattern, track, column, line1 + 1).size(), 0);
    QCOMPARE(automationService.pitchBendAutomationsByColumn(pattern, track, column).size(), 1);
    QCOMPARE(automationService.pitchBendAutomationsByColumn(pattern, track, column + 1).size(), 0);
    QCOMPARE(automationService.pitchBendAutomationsByTrack(pattern, track).size(), 1);
    QCOMPARE(automationService.pitchBendAutomationsByTrack(pattern, track).size(), 1);
    QCOMPARE(automationService.pitchBendAutomationsByPattern(pattern).size(), 1);
    QCOMPARE(automationService.pitchBendAutomationsByPattern(pattern + 1).size(), 0);
    QCOMPARE(automationService.pitchBendAutomations().size(), 1);

    QVERIFY(automationService.hasAutomations(pattern, track, column, line0));
    QVERIFY(automationService.hasAutomations(pattern, track, column, line1));
    QVERIFY(automationService.hasAutomations(pattern, track, column, (line0 + line1) / 2));

    id = automationService.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, comment);
    QCOMPARE(id, 2);
    id = automationService.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, comment);
    QCOMPARE(id, 3);
}

void AutomationServiceTest::test_deletePitchBendAutomation_shouldDeleteAutomation()
{
    AutomationService automationService;

    automationService.addPitchBendAutomation(0, 0, 0, 0, 1, 0, 1, {});
    QVERIFY(!automationService.pitchBendAutomations().empty());
    auto automation = automationService.pitchBendAutomations().at(0);
    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    automationService.deletePitchBendAutomation(automation);

    QVERIFY(automationService.pitchBendAutomations().empty());
    QCOMPARE(lineDataChangedSpy.count(), automation.interpolation().line1 - automation.interpolation().line0 + 1);
}

void AutomationServiceTest::test_automationWeight_midiCc_shouldCalculateCorrectWeight()
{
    AutomationService automationService;

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
    QVERIFY(std::fabs(automationService.automationWeight(pattern, track, column, (line0 + line1) / 2) - 0.5) < 0.01);
    QCOMPARE(automationService.automationWeight(pattern, track, column, line1), 1);
}

void AutomationServiceTest::test_automationWeight_pitchBendUp_shouldCalculateCorrectWeight()
{
    AutomationService automationService;

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 line0 = 0;
    quint8 line1 = 16;
    int value0 = 0;
    int value1 = 100;

    automationService.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, {});

    QVERIFY(std::fabs(automationService.automationWeight(pattern, track, column, line0) - 0.5) < 0.01);
    QVERIFY(std::fabs(automationService.automationWeight(pattern, track, column, (line0 + line1) / 2) - 0.75) < 0.01);
    QCOMPARE(automationService.automationWeight(pattern, track, column, line1), 1);
}

void AutomationServiceTest::test_automationWeight_pitchBendDown_shouldCalculateCorrectWeight()
{
    AutomationService automationService;

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 line0 = 0;
    quint8 line1 = 16;
    int value0 = 0;
    int value1 = -100;

    automationService.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, {});

    QVERIFY(std::fabs(automationService.automationWeight(pattern, track, column, line0) - 0.5) < 0.01);
    QVERIFY(std::fabs(automationService.automationWeight(pattern, track, column, (line0 + line1) / 2) - 0.25) < 0.01);
    QCOMPARE(automationService.automationWeight(pattern, track, column, line1), 0);
}

void AutomationServiceTest::test_renderToEventsByLine_shouldRenderToEvents()
{
    AutomationService automationService;

    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
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

void AutomationServiceTest::test_renderToEventsByLine_disableAutomation_shouldNotRenderEvents()
{
    AutomationService automationService;
    automationService.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {});
    QVERIFY(!automationService.renderToEventsByLine(0, 0, 0, 0, 0).empty());
    auto automation = automationService.midiCcAutomations().at(0);
    automation.setEnabled(false);
    automationService.updateMidiCcAutomation(automation);
    QVERIFY(automationService.renderToEventsByLine(0, 0, 0, 0, 0).empty());
}

void AutomationServiceTest::test_renderMidiCcToEventsByLine_withModulation_shouldRenderModulatedEvents()
{
    AutomationService automationService;

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 4;
    quint8 value0 = 64;
    quint8 value1 = 64;
    const auto automationId = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {});
    automationService.addMidiCcModulation(automationId, 1, 50.0f, false);

    const auto tick = 0;
    // Line 0: Base 64, Phase 0, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->midiCcData()->value(), 64);
    // Line 1: Base 64, Phase 0.25, Sine 1, Modulation 32
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 1, tick).at(0)->midiCcData()->value(), 96);
    // Line 2: Base 64, Phase 0.5, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 2, tick).at(0)->midiCcData()->value(), 64);
    // Line 3: Base 64, Phase 0.75, Sine -1, Modulation -32
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 3, tick).at(0)->midiCcData()->value(), 32);
    // Line 4: Base 64, Phase 1, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 4, tick).at(0)->midiCcData()->value(), 64);
}

void AutomationServiceTest::test_renderMidiCcToEventsByLine_withInvertedModulation_shouldRenderModulatedEvents()
{
    AutomationService automationService;

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 4;
    quint8 value0 = 64;
    quint8 value1 = 64;
    const auto automationId = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {});
    automationService.addMidiCcModulation(automationId, 1, 50.0f, true);

    const auto tick = 0;
    // Line 0: Base 64, Phase 0, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->midiCcData()->value(), 64);
    // Line 1: Base 64, Phase 0.25, Sine -1, Modulation -32
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 1, tick).at(0)->midiCcData()->value(), 32);
    // Line 2: Base 64, Phase 0.5, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 2, tick).at(0)->midiCcData()->value(), 64);
    // Line 3: Base 64, Phase 0.75, Sine 1, Modulation 32
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 3, tick).at(0)->midiCcData()->value(), 96);
    // Line 4: Base 64, Phase 1, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 4, tick).at(0)->midiCcData()->value(), 64);
}

void AutomationServiceTest::test_renderToEventsByColumn_shouldRenderToEvents()
{
    AutomationService automationService;

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
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
        QCOMPARE(events.at(i)->midiCcData()->value(), static_cast<uint8_t>(std::round(interpolator.getValue(line))));
        QCOMPARE(events.at(i)->midiCcData()->track(), track);
        QCOMPARE(events.at(i)->midiCcData()->column(), column);
    }
}

void AutomationServiceTest::test_renderToEventsByColumn_shouldPruneRepeatingEvents()
{
    AutomationService automationService;

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 120, 10, 20, {});

    const auto tick = 666;
    const auto ticksPerLine = 24;
    const auto events = automationService.renderToEventsByColumn(pattern, track, column, tick, ticksPerLine);
    QCOMPARE(automationService.renderToEventsByColumn(pattern, track, column, tick, tick).size(), 11);
}

void AutomationServiceTest::test_renderToEventsByColumn_disableAutomation_shouldNotRenderEvents()
{
    AutomationService automationService;
    automationService.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {});
    QVERIFY(!automationService.renderToEventsByColumn(0, 0, 0, 0, 24).empty());
    auto automation = automationService.midiCcAutomations().at(0);
    automation.setEnabled(false);
    automationService.updateMidiCcAutomation(automation);
    QVERIFY(automationService.renderToEventsByColumn(0, 0, 0, 0, 24).empty());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutomationServiceTest)
