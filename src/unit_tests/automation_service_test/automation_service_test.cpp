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
#include "../../application/service/property_service.hpp"
#include "../../domain/midi/midi_cc_automation.hpp"
#include "../../domain/tracker/interpolator.hpp"

#include <QSignalSpy>
#include <QTest>

Q_DECLARE_METATYPE(noteahead::Position)

namespace noteahead {

void AutomationServiceTest::initTestCase()
{
    qRegisterMetaType<noteahead::Position>("Position");
}

void AutomationServiceTest::test_addMidiCcAutomation_shouldAddAutomation()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
    const auto comment = "Comment";
    auto id = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment, true, 8, 0);
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

    id = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment, true, 8, 0);
    QCOMPARE(id, 2);
    id = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment, true, 8, 0);
    QCOMPARE(id, 3);
}

void AutomationServiceTest::test_deleteMidiCcAutomation_shouldDeleteAutomation()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    automationService.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {}, true, 8, 0);
    QVERIFY(!automationService.midiCcAutomations().empty());
    auto automation = automationService.midiCcAutomations().at(0);
    QSignalSpy lineDataChangedSpy { &automationService, &AutomationService::lineDataChanged };
    automationService.deleteMidiCcAutomation(automation);

    QVERIFY(automationService.midiCcAutomations().empty());
    QCOMPARE(lineDataChangedSpy.count(), automation.interpolation().line1 - automation.interpolation().line0 + 1);
}

void AutomationServiceTest::test_addPitchBendAutomation_shouldAddAutomation()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

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
    AutomationService automationService { std::make_shared<PropertyService>() };

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
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 16;
    quint8 value0 = 0;
    quint8 value1 = 127;

    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);

    QCOMPARE(automationService.automationWeight(pattern, track, column, line0), 0);
    QVERIFY(std::fabs(automationService.automationWeight(pattern, track, column, (line0 + line1) / 2) - 0.5) < 0.01);
    QCOMPARE(automationService.automationWeight(pattern, track, column, line1), 1);
}

void AutomationServiceTest::test_hasAutomations_disabled_shouldReportNone()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    const auto midiCcId = automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 8, 0, 127, {}, false, 8, 0);
    const auto pitchBendId = automationService.addPitchBendAutomation(pattern, track, column, 0, 8, -100, 100, {}, false);
    QVERIFY(midiCcId);
    QVERIFY(pitchBendId);

    // A disabled automation is not there as far as the display is concerned. It used to still count,
    // which left the column tinted at weight zero, i.e. solid red, after switching it off.
    QVERIFY(!automationService.hasAutomations(pattern, track, column, 4));
    QCOMPARE(automationService.automationWeight(pattern, track, column, 4), 0.0);
    QVERIFY(automationService.automationCurves(pattern, track, column, 0, 8).empty());
}

void AutomationServiceTest::test_automationCurves_midiCc_shouldFollowInterpolation()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 16, 0, 127, {}, true, 8, 0);

    const auto curves = automationService.automationCurves(pattern, track, column, 0, 16);
    QCOMPARE(curves.size(), size_t { 1 });
    QCOMPARE(curves.at(0).isPitchBend, false);
    QCOMPARE(curves.at(0).values.size(), size_t { 17 });

    // Normalized to 0..1 against the controller's own range, so both kinds share one axis
    QVERIFY(curves.at(0).values.at(0).has_value());
    QVERIFY(std::fabs(*curves.at(0).values.at(0) - 0.0) < 0.01);
    QVERIFY(std::fabs(*curves.at(0).values.at(8) - 0.5) < 0.01);
    QVERIFY(std::fabs(*curves.at(0).values.at(16) - 1.0) < 0.01);
}

void AutomationServiceTest::test_automationCurves_outsideRange_shouldLeaveLinesUnset()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    automationService.addMidiCcAutomation(pattern, track, column, 64, 4, 8, 0, 127, {}, true, 8, 0);

    // Lines the automation does not cover stay unset, which is what makes the drawn trace stop at
    // its ends rather than running to the edge of the pattern
    const auto curves = automationService.automationCurves(pattern, track, column, 0, 12);
    QCOMPARE(curves.size(), size_t { 1 });
    QVERIFY(!curves.at(0).values.at(0).has_value());
    QVERIFY(!curves.at(0).values.at(3).has_value());
    QVERIFY(curves.at(0).values.at(4).has_value());
    QVERIFY(curves.at(0).values.at(8).has_value());
    QVERIFY(!curves.at(0).values.at(9).has_value());
}

void AutomationServiceTest::test_automationCurves_sineModulation_shouldOscillate()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    // Flat interpolation at the mid point, so everything the curve does comes from the modulation
    const auto id = automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 32, 64, 64, {}, true, 8, 0);
    automationService.addMidiCcModulation(id, 0, 1, 50.0f, 0.0f, false); // One sine cycle at 50%

    const auto curves = automationService.automationCurves(pattern, track, column, 0, 32);
    QCOMPARE(curves.size(), size_t { 1 });
    const auto & values = curves.at(0).values;

    // A full cycle has to rise above the mid point and fall below it again
    double minimum = 1.0, maximum = 0.0;
    for (auto && value : values) {
        QVERIFY(value.has_value());
        minimum = std::min(minimum, *value);
        maximum = std::max(maximum, *value);
    }
    QVERIFY2(maximum > 0.6, qPrintable(QString { "Maximum %1" }.arg(maximum)));
    QVERIFY2(minimum < 0.4, qPrintable(QString { "Minimum %1" }.arg(minimum)));
}

void AutomationServiceTest::test_automationCurves_severalAutomations_shouldStayApart()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 8, 0, 127, {}, true, 8, 0);
    automationService.addMidiCcAutomation(pattern, track, column, 74, 0, 8, 127, 0, {}, true, 8, 0);
    automationService.addPitchBendAutomation(pattern, track, column, 0, 8, -100, 100, {}, true);

    // Each automation keeps its own trace instead of being averaged into one value per line
    const auto curves = automationService.automationCurves(pattern, track, column, 0, 8);
    QCOMPARE(curves.size(), size_t { 3 });
    QCOMPARE(curves.at(2).isPitchBend, true);
    QVERIFY(std::fabs(*curves.at(0).values.at(0) - 0.0) < 0.01);
    QVERIFY(std::fabs(*curves.at(1).values.at(0) - 1.0) < 0.01);
    QVERIFY(std::fabs(*curves.at(2).values.at(0) - 0.0) < 0.01); // -100% maps to the left edge
    QVERIFY(std::fabs(*curves.at(2).values.at(4) - 0.5) < 0.01); // Centre
}

void AutomationServiceTest::test_automationWeight_pitchBendUp_shouldCalculateCorrectWeight()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

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
    AutomationService automationService { std::make_shared<PropertyService>() };

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
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);
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
    AutomationService automationService { std::make_shared<PropertyService>() };
    automationService.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {}, true, 8, 0);
    QVERIFY(!automationService.renderToEventsByLine(0, 0, 0, 0, 0).empty());
    auto automation = automationService.midiCcAutomations().at(0);
    automation.setEnabled(false);
    automationService.updateMidiCcAutomation(automation);
    QVERIFY(automationService.renderToEventsByLine(0, 0, 0, 0, 0).empty());
}

void AutomationServiceTest::test_renderMidiCcToEventsByLine_withModulation_shouldRenderModulatedEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 4;
    quint8 value0 = 64;
    quint8 value1 = 64;
    const auto automationId = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);
    automationService.addMidiCcModulation(automationId, 0, 1, 50.0f, 0.0f, false);

    const auto tick = 0;
    // Line 0: Base 64, Phase 0, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->midiCcData()->value(), 64);
    // Line 1: Base 64, Phase 0.25, Sine 1, Modulation 0.5 * 127 = 63.5
    // 64 + 63.5 = 127.5 -> 128 (clamped to 127)
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 1, tick).at(0)->midiCcData()->value(), 127);
    // Line 2: Base 64, Phase 0.5, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 2, tick).at(0)->midiCcData()->value(), 64);
    // Line 3: Base 64, Phase 0.75, Sine -1, Modulation -0.5 * 127 = -63.5
    // 64 - 63.5 = 0.5 -> 1
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 3, tick).at(0)->midiCcData()->value(), 1);
    // Line 4: Base 64, Phase 1, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 4, tick).at(0)->midiCcData()->value(), 64);
}

void AutomationServiceTest::test_renderMidiCcToEventsByLine_withRandomModulation_shouldRenderModulatedEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 4;
    quint8 value0 = 64;
    quint8 value1 = 64;
    // ID will be 1
    const auto automationId = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);
    // Type 1 = Random, 1 cycle, 100% amplitude
    automationService.addMidiCcModulation(automationId, 1, 1, 100.0f, 0.0f, false);

    const auto tick = 0;
    // For automationId = 1 and sampleIndex = 0, std::mt19937 seeded with 1:
    // first value from dist(-1, 1) is roughly -0.131538
    // value = 64 + (-0.131538) * 127 = 64 - 16.7 = 47.3 -> 47
    const auto val0 = automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->midiCcData()->value();
    QVERIFY(val0 != 64);

    // Should stay same for line 1, 2, 3 (within same cycle)
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 1, tick).at(0)->midiCcData()->value(), val0);
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 2, tick).at(0)->midiCcData()->value(), val0);
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 3, tick).at(0)->midiCcData()->value(), val0);

    // Return to 0 at the end (line 4, phase 1.0)
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 4, tick).at(0)->midiCcData()->value(), 64);
}

void AutomationServiceTest::test_renderMidiCcToEventsByLine_withInvertedModulation_shouldRenderModulatedEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 4;
    quint8 value0 = 64;
    quint8 value1 = 64;
    const auto automationId = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);
    automationService.addMidiCcModulation(automationId, 0, 1, 50.0f, 0.0f, true);

    const auto tick = 0;
    // Line 0: Base 64, Phase 0, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->midiCcData()->value(), 64);
    // Line 1: Base 64, Phase 0.25, Sine -1, Modulation -0.5 * 127 = -63.5
    // 64 - 63.5 = 0.5 -> 1
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 1, tick).at(0)->midiCcData()->value(), 1);
    // Line 2: Base 64, Phase 0.5, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 2, tick).at(0)->midiCcData()->value(), 64);
    // Line 3: Base 64, Phase 0.75, Sine 1, Modulation 0.5 * 127 = 63.5
    // 64 + 63.5 = 127.5 -> 128 (clamped to 127)
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 3, tick).at(0)->midiCcData()->value(), 127);
    // Line 4: Base 64, Phase 1, Sine 0, Modulation 0
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 4, tick).at(0)->midiCcData()->value(), 64);
}

void AutomationServiceTest::test_renderMidiCcToEventsByLine_withOffset_shouldRenderOffsetEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 0;
    quint8 line1 = 4;
    quint8 value0 = 64;
    quint8 value1 = 64;
    const auto automationId = automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);

    // Test positive offset (+50%)
    automationService.addMidiCcModulation(automationId, 0, 0, 0.0f, 50.0f, false);
    const auto tick = 0;
    // 64 + 0.5 * 127 = 64 + 63.5 = 127.5 -> 128 (clamped to 127)
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->midiCcData()->value(), 127);

    // Test negative offset (-50%)
    automationService.addMidiCcModulation(automationId, 0, 0, 0.0f, -50.0f, false);
    // 64 - 0.5 * 127 = 64 - 63.5 = 0.5 -> 1
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->midiCcData()->value(), 1);
}

void AutomationServiceTest::test_renderPitchBendToEventsByLine_withModulation_shouldRenderModulatedEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 line0 = 0;
    quint8 line1 = 4;
    int value0 = 0;
    int value1 = 0;
    const auto automationId = automationService.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, {});
    automationService.addPitchBendModulation(automationId, 0, 1, 50.0f, 0.0f, false);

    const auto tick = 0;
    // Line 0: Base 0, Phase 0, Sine 0, Modulation 0 -> percentage 0 -> normalized 8192 / 16383
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 0, tick).at(0)->pitchBendData()->normalizedValue(), 8192.0 / 16383.0);
    // Line 1: Base 0, Phase 0.25, Sine 1, Modulation 0.5 * 100 = 50 -> percentage 50 -> normalized 12287 / 16383
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 1, tick).at(0)->pitchBendData()->normalizedValue(), 12287.0 / 16383.0);
    // Line 2: Base 0, Phase 0.5, Sine 0, Modulation 0 -> percentage 0 -> normalized 8192 / 16383
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 2, tick).at(0)->pitchBendData()->normalizedValue(), 8192.0 / 16383.0);
    // Line 3: Base 0, Phase 0.75, Sine -1, Modulation -0.5 * 100 = -50 -> percentage -50 -> normalized 4096 / 16383
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 3, tick).at(0)->pitchBendData()->normalizedValue(), 4096.0 / 16383.0);
    // Line 4: Base 0, Phase 1, Sine 0, Modulation 0 -> percentage 0 -> normalized 8192 / 16383
    QCOMPARE(automationService.renderToEventsByLine(pattern, track, column, 4, tick).at(0)->pitchBendData()->normalizedValue(), 8192.0 / 16383.0);
}

void AutomationServiceTest::test_renderMidiCcToEventsByColumn_withEventsPerBeatAndLineOffset_shouldSkipEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };
    // 0 to 15 lines (16 lines total)
    // linesPerBeat = 8.
    // eventsPerBeat = 1 -> should fire every 8 lines.
    // lineOffset = 0 -> lines 0, 8. And last line 15.
    automationService.addMidiCcAutomation(0, 0, 0, 1, 0, 15, 0, 100, {}, true, 1, 0);

    const auto events = automationService.renderToEventsByColumn(0, 0, 0, 0, 1, 8);

    QCOMPARE(events.size(), 3);
    QCOMPARE(events.at(0)->tick(), 0);
    QCOMPARE(events.at(1)->tick(), 8);
    QCOMPARE(events.at(2)->tick(), 15);

    // Now change lineOffset to 7 -> lines 7, 15.
    auto automation = automationService.midiCcAutomations().at(0);
    automation.setLineOffset(7);
    automationService.updateMidiCcAutomation(automation);

    const auto events2 = automationService.renderToEventsByColumn(0, 0, 0, 0, 1, 8);
    QCOMPARE(events2.size(), 2);
    QCOMPARE(events2.at(0)->tick(), 7);
    QCOMPARE(events2.at(1)->tick(), 15);

    // Test max events per beat
    automation.setEventsPerBeat(8);
    automation.setLineOffset(0);
    automationService.updateMidiCcAutomation(automation);
    const auto events3 = automationService.renderToEventsByColumn(0, 0, 0, 0, 1, 8);
    QCOMPARE(events3.size(), 16);
}

void AutomationServiceTest::test_renderToEventsByColumn_shouldRenderToEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);

    const auto tick = 666;
    const auto ticksPerLine = 24;
    const auto events = automationService.renderToEventsByColumn(pattern, track, column, tick, ticksPerLine, 8);
    QCOMPARE(automationService.renderToEventsByColumn(pattern, track, column, tick, tick, 8).size(), line1 - line0 + 1);
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

void AutomationServiceTest::test_renderToEventsByColumn_curve_shouldFollowCurve()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0;
    const quint64 track = 1;
    const quint64 column = 2;
    const quint8 controller = 64;
    const quint8 line0 = 0;
    const quint8 line1 = 8;
    const quint8 value0 = 0;
    const quint8 value1 = 100;
    automationService.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, {}, true, 8, 0);

    auto automation = automationService.midiCcAutomations().at(0);
    auto interpolation = automation.interpolation();
    interpolation.curve = Interpolator::CurveType::Exponential;
    automation.setInterpolation(interpolation);
    automationService.updateMidiCcAutomation(automation);

    const auto events = automationService.renderToEventsByColumn(pattern, track, column, 0, 24, 8);
    const Interpolator interpolator {
        static_cast<size_t>(line0),
        static_cast<size_t>(line1),
        static_cast<double>(value0),
        static_cast<double>(value1),
        Interpolator::CurveType::Exponential
    };
    QCOMPARE(events.size(), static_cast<size_t>(line1 - line0 + 1));
    for (size_t line = line0; line <= line1; line++) {
        const auto i = line - line0;
        QCOMPARE(events.at(i)->midiCcData()->value(), static_cast<uint8_t>(std::round(interpolator.getValue(line))));
    }
    // The midpoint of an exponential ramp sits well below the linear one
    QCOMPARE(events.at(4)->midiCcData()->value(), 25);
}

void AutomationServiceTest::test_renderToEventsByColumn_shouldPruneRepeatingEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    quint64 pattern = 0;
    quint64 track = 1;
    quint64 column = 2;
    automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 120, 10, 20, {}, true, 8, 0);

    const auto tick = 666;
    const auto ticksPerLine = 24;
    const auto events = automationService.renderToEventsByColumn(pattern, track, column, tick, ticksPerLine, 8);
    QCOMPARE(automationService.renderToEventsByColumn(pattern, track, column, tick, tick, 8).size(), 11);
}

void AutomationServiceTest::test_renderToEventsByColumn_disableAutomation_shouldNotRenderEvents()
{
    AutomationService automationService { std::make_shared<PropertyService>() };
    automationService.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {}, true, 8, 0);
    QVERIFY(!automationService.renderToEventsByColumn(0, 0, 0, 0, 24, 8).empty());
    auto automation = automationService.midiCcAutomations().at(0);
    automation.setEnabled(false);
    automationService.updateMidiCcAutomation(automation);
    QVERIFY(automationService.renderToEventsByColumn(0, 0, 0, 0, 24, 8).empty());
}

void AutomationServiceTest::test_setMidiCcAutomationCurve_shouldChangeTheRamp()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    const auto id = automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 16, 0, 127, {}, true, 8, 0);

    // Linear to begin with: the automation is born on the default curve, whatever the add call omits
    QVERIFY(std::fabs(*automationService.automationCurves(pattern, track, column, 0, 16).at(0).values.at(8) - 0.5) < 0.01);

    automationService.setMidiCcAutomationCurve(id, static_cast<int>(Interpolator::CurveType::Exponential));

    // An exponential ramp is below the linear one at the midpoint, and still pinned at both ends
    const auto values = automationService.automationCurves(pattern, track, column, 0, 16).at(0).values;
    QVERIFY2(*values.at(8) < 0.4, qPrintable(QString { "Midpoint %1" }.arg(*values.at(8))));
    QVERIFY(std::fabs(*values.at(0) - 0.0) < 0.01);
    QVERIFY(std::fabs(*values.at(16) - 1.0) < 0.01);
}

void AutomationServiceTest::test_setPitchBendAutomationCurve_shouldChangeTheRamp()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    const auto id = automationService.addPitchBendAutomation(pattern, track, column, 0, 16, -100, 100, {}, true);

    QVERIFY(std::fabs(*automationService.automationCurves(pattern, track, column, 0, 16).at(0).values.at(8) - 0.5) < 0.01);

    automationService.setPitchBendAutomationCurve(id, static_cast<int>(Interpolator::CurveType::Exponential));

    const auto values = automationService.automationCurves(pattern, track, column, 0, 16).at(0).values;
    QVERIFY2(*values.at(8) < 0.4, qPrintable(QString { "Midpoint %1" }.arg(*values.at(8))));
    QVERIFY(std::fabs(*values.at(0) - 0.0) < 0.01);
    QVERIFY(std::fabs(*values.at(16) - 1.0) < 0.01);
}

void AutomationServiceTest::test_setAutomationCurve_unknownId_shouldDoNothing()
{
    AutomationService automationService { std::make_shared<PropertyService>() };

    const quint64 pattern = 0, track = 1, column = 2;
    automationService.addMidiCcAutomation(pattern, track, column, 64, 0, 16, 0, 127, {}, true, 8, 0);

    automationService.setMidiCcAutomationCurve(12345, static_cast<int>(Interpolator::CurveType::Exponential));
    automationService.setPitchBendAutomationCurve(12345, static_cast<int>(Interpolator::CurveType::Exponential));

    // Still linear: an id that matches nothing must not fall through onto some other automation
    QVERIFY(std::fabs(*automationService.automationCurves(pattern, track, column, 0, 16).at(0).values.at(8) - 0.5) < 0.01);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutomationServiceTest)
