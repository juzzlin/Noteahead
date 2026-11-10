// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#include "automation_take_test.hpp"

#include "../../domain/tracker/automation_take.hpp"

#include <QTest>

namespace noteahead {

using namespace AutomationTake;

namespace {

ParameterAutomation rampAutomation(size_t line0, size_t line1, double value0, double value1)
{
    const ParameterAutomation::Target target { ParameterAutomation::Target::Owner::Device, "pad", "pad1_x_position" };
    return { 1, AutomationLocation { 0, 0, 0 }, target, ParameterAutomation::Mode::Absolute, { line0, line1, value0, value1 }, {}, {}, true };
}

} // namespace

void AutomationTakeTest::test_split_newPattern_shouldStartAPiece()
{
    const auto pieces = split({ { 0, 62.0, 0.1 }, { 0, 63.5, 0.2 }, { 1, 0.2, 0.3 } });
    QCOMPARE(pieces.size(), size_t { 2 });
    QCOMPARE(pieces.at(0).size(), size_t { 2 });
    QCOMPARE(pieces.at(1).front().pattern, size_t { 1 });
}

void AutomationTakeTest::test_split_lineGoesBack_shouldStartAPiece()
{
    // The same pattern again, as a loop plays it
    const auto pieces = split({ { 2, 14.0, 0.1 }, { 2, 15.9, 0.2 }, { 2, 0.1, 0.3 }, { 2, 0.5, 0.4 } });
    QCOMPARE(pieces.size(), size_t { 2 });
    QCOMPARE(pieces.at(1).size(), size_t { 2 });
}

void AutomationTakeTest::test_simplify_straightLine_shouldKeepTheEnds()
{
    const auto nodes = simplify({ { 0.0, 0.0 }, { 1.0, 0.25 }, { 2.0, 0.5 }, { 3.0, 0.75 }, { 4.0, 1.0 } }, 0.001);
    QCOMPARE(nodes, (AutomationCurve::NodeList { { 0.0, 0.0 }, { 4.0, 1.0 } }));
}

void AutomationTakeTest::test_simplify_corner_shouldKeepIt()
{
    const auto nodes = simplify({ { 0.0, 0.0 }, { 1.0, 0.5 }, { 2.0, 1.0 }, { 3.0, 0.5 }, { 4.0, 0.0 } }, 0.01);
    QCOMPARE(nodes, (AutomationCurve::NodeList { { 0.0, 0.0 }, { 2.0, 1.0 }, { 4.0, 0.0 } }));
}

void AutomationTakeTest::test_curve_shouldSpanTheLinesPlayed()
{
    const auto curve = AutomationTake::curve({ { 0, 4.3, 0.5 }, { 0, 5.0, 1.0 }, { 0, 6.5, 0.5 } }, 64, 0.001);
    QVERIFY(curve);
    QCOMPARE(curve->line0, size_t { 4 });
    QCOMPARE(curve->line1, size_t { 7 });
    QCOMPARE(curve->curve, Interpolator::CurveType::Drawn);
    QCOMPARE(curve->value0, 0.5);
    QCOMPARE(curve->value1, 0.5);
    // Held at the start until the first move, and at the end after the last
    QCOMPARE(curve->drawnCurve.valueAt(4.3), 0.5);
    QCOMPARE(curve->drawnCurve.valueAt(5.0), 1.0);
    QCOMPARE(curve->drawnCurve.valueAt(6.8), 0.5);
}

void AutomationTakeTest::test_curve_pastTheEnd_shouldStopAtTheLastLine()
{
    const auto curve = AutomationTake::curve({ { 0, 14.0, 0.2 }, { 0, 15.5, 0.9 } }, 16, 0.001);
    QVERIFY(curve);
    QCOMPARE(curve->line1, size_t { 15 });
    QCOMPARE(curve->drawnCurve.nodes().back().line, 15.0);
}

void AutomationTakeTest::test_curve_lastLine_shouldBeNothing()
{
    QVERIFY(!AutomationTake::curve({ { 0, 15.2, 0.2 }, { 0, 15.6, 0.9 } }, 16, 0.001));
    QVERIFY(!AutomationTake::curve({}, 16, 0.001));
}

void AutomationTakeTest::test_remainderOutside_noOverlap_shouldKeepItAsIs()
{
    const auto automation = rampAutomation(0, 8, 0.0, 1.0);
    const auto remainder = remainderOutside(automation, 9, 12);
    QCOMPARE(remainder.size(), size_t { 1 });
    QVERIFY(remainder.front() == automation);
}

void AutomationTakeTest::test_remainderOutside_covered_shouldBeNothing()
{
    QVERIFY(remainderOutside(rampAutomation(4, 8, 0.0, 1.0), 3, 9).empty());
    // A line on either side is too little to be an automation
    QVERIFY(remainderOutside(rampAutomation(4, 8, 0.0, 1.0), 5, 7).empty());
}

void AutomationTakeTest::test_remainderOutside_middle_shouldKeepBothSidesAsTheyPlay()
{
    const auto remainder = remainderOutside(rampAutomation(0, 16, 0.0, 1.0), 6, 9);
    QCOMPARE(remainder.size(), size_t { 2 });
    const auto & left = remainder.at(0).interpolation();
    const auto & right = remainder.at(1).interpolation();
    QCOMPARE(left.line0, size_t { 0 });
    QCOMPARE(left.line1, size_t { 5 });
    QCOMPARE(right.line0, size_t { 10 });
    QCOMPARE(right.line1, size_t { 16 });
    QCOMPARE(left.curve, Interpolator::CurveType::Drawn);
    QCOMPARE(left.value1, 5.0 / 16.0);
    QCOMPARE(right.value0, 10.0 / 16.0);
    QCOMPARE(right.value1, 1.0);
}

void AutomationTakeTest::test_steps_shouldHoldEachSettingUpToTheNearestLine()
{
    const auto stepped = steps({ { 0, 2.4, 127.0 }, { 0, 3.0, 127.0 }, { 0, 5.6, 0.0 } });
    QCOMPARE(stepped.size(), size_t { 3 });
    QCOMPARE(stepped.at(0).line, 2.0);
    QCOMPARE(stepped.at(1).line, 5.5);
    QCOMPARE(stepped.at(1).value, 127.0);
    QCOMPARE(stepped.at(2).line, 6.0);
    QCOMPARE(stepped.at(2).value, 0.0);
}

void AutomationTakeTest::test_steps_twiceOnOneLine_shouldKeepTheLast()
{
    const auto stepped = steps({ { 0, 1.0, 0.0 }, { 0, 4.1, 127.0 }, { 0, 4.2, 0.0 } });
    QCOMPARE(stepped.back().line, 4.0);
    QCOMPARE(stepped.back().value, 0.0);
}

void AutomationTakeTest::test_remainderOutside_midiCc_shouldKeepItsValuesInSteps()
{
    const MidiCcAutomation automation { 1, AutomationLocation { 0, 0, 0 }, 1, { 0, 10, 0, 100 }, {} };
    const auto remainder = remainderOutside(automation, 4, 6);
    QCOMPARE(remainder.size(), size_t { 2 });
    QCOMPARE(remainder.at(0).interpolation().line1, size_t { 3 });
    QCOMPARE(remainder.at(0).interpolation().value1, uint8_t { 30 });
    QCOMPARE(remainder.at(1).interpolation().line0, size_t { 7 });
    QCOMPARE(remainder.at(1).interpolation().value0, uint8_t { 70 });
    QCOMPARE(remainder.at(1).controller(), uint8_t { 1 });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutomationTakeTest)
