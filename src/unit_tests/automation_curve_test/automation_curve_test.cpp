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

#include "automation_curve_test.hpp"

#include "../../domain/tracker/automation_curve.hpp"

#include <QTest>

namespace noteahead {

using Node = AutomationCurve::Node;

//! 0 at line 0, up to 1 at line 4, down to 0.5 at line 8
static AutomationCurve threeNodes()
{
    return AutomationCurve { { { 0, 0 }, { 4, 1 }, { 8, 0.5 } } };
}

void AutomationCurveTest::test_constructor_unsortedNodes_shouldSortByLine()
{
    const AutomationCurve curve { { { 8, 0.5 }, { 0, 0 }, { 4, 1 } } };

    QCOMPARE(curve.nodes(), threeNodes().nodes());
}

void AutomationCurveTest::test_ramp_shouldHaveTwoNodesAtTheEnds()
{
    const auto curve = AutomationCurve::ramp(2, 10, 6, 30);

    QCOMPARE(curve.nodes(), (AutomationCurve::NodeList { { 2, 10 }, { 6, 30 } }));
    QCOMPARE(curve.valueAt(4), 20.0);
}

void AutomationCurveTest::test_ramp_zeroLength_shouldHaveOneNode()
{
    const auto curve = AutomationCurve::ramp(3, 10, 3, 30);

    QCOMPARE(curve.nodes().size(), size_t { 1 });
    QCOMPARE(curve.valueAt(3), 10.0);
}

void AutomationCurveTest::test_valueAt_betweenNodes_shouldBeLinear()
{
    const auto curve = threeNodes();

    QCOMPARE(curve.valueAt(0), 0.0);
    QCOMPARE(curve.valueAt(1), 0.25);
    QCOMPARE(curve.valueAt(4), 1.0);
    QCOMPARE(curve.valueAt(6), 0.75);
    QCOMPARE(curve.valueAt(7.5), 0.5625);
    QCOMPARE(curve.valueAt(8), 0.5);
}

void AutomationCurveTest::test_valueAt_outside_shouldHoldTheEnds()
{
    const auto curve = threeNodes();

    QCOMPARE(curve.valueAt(-3), 0.0);
    QCOMPARE(curve.valueAt(100), 0.5);
}

void AutomationCurveTest::test_valueAt_empty_shouldBeZero()
{
    QCOMPARE(AutomationCurve {}.valueAt(3), 0.0);
}

void AutomationCurveTest::test_insert_betweenNodes_shouldKeepOrder()
{
    auto curve = threeNodes();

    QCOMPARE(curve.insert(6, 0.1), std::optional<size_t> { 2 });
    QCOMPARE(curve.insert(2, 0.9), std::optional<size_t> { 1 });

    QCOMPARE(curve.nodes(), (AutomationCurve::NodeList { { 0, 0 }, { 2, 0.9 }, { 4, 1 }, { 6, 0.1 }, { 8, 0.5 } }));
}

void AutomationCurveTest::test_insert_offLine_shouldKeepTheLine()
{
    auto curve = threeNodes();

    QCOMPARE(curve.insert(4.5, 0.2), std::optional<size_t> { 2 });
    QCOMPARE(curve.nodes().at(2), (Node { 4.5, 0.2 }));
}

void AutomationCurveTest::test_insert_atOrPastEnds_shouldRefuse()
{
    auto curve = threeNodes();

    QVERIFY(!curve.insert(0, 1).has_value());
    QVERIFY(!curve.insert(-1, 1).has_value());
    QVERIFY(!curve.insert(8, 1).has_value());
    QVERIFY(!curve.insert(9, 1).has_value());
    QCOMPARE(curve.nodes().size(), size_t { 3 });
}

void AutomationCurveTest::test_insert_tooCloseToNode_shouldRefuse()
{
    auto curve = threeNodes();

    QVERIFY(!curve.insert(4, 0.3).has_value());
    QVERIFY(!curve.insert(4 + AutomationCurve::minNodeGap / 2, 0.3).has_value());
    QVERIFY(curve.insert(4 + AutomationCurve::minNodeGap, 0.3).has_value());
}

void AutomationCurveTest::test_insert_onCurve_shouldNotChangeTheSound()
{
    auto curve = threeNodes();
    const auto before = curve;

    curve.insert(6, curve.valueAt(6));

    for (double line = 0; line <= 8; line += 0.25) {
        QCOMPARE(curve.valueAt(line), before.valueAt(line));
    }
}

void AutomationCurveTest::test_remove_end_shouldRefuse()
{
    auto curve = threeNodes();

    QVERIFY(!curve.remove(0));
    QVERIFY(!curve.remove(2));
    QVERIFY(!curve.remove(3));
    QCOMPARE(curve.nodes().size(), size_t { 3 });
}

void AutomationCurveTest::test_remove_inner_shouldRemove()
{
    auto curve = threeNodes();

    QVERIFY(curve.remove(1));

    QCOMPARE(curve.nodes(), (AutomationCurve::NodeList { { 0, 0 }, { 8, 0.5 } }));
}

void AutomationCurveTest::test_move_end_shouldOnlyChangeValue()
{
    auto curve = threeNodes();

    curve.move(0, 3, 0.7);
    curve.move(2, 5, 0.1);

    QCOMPARE(curve.nodes().front(), (Node { 0, 0.7 }));
    QCOMPARE(curve.nodes().back(), (Node { 8, 0.1 }));
}

void AutomationCurveTest::test_move_inner_shouldStopAtNeighbours()
{
    auto curve = threeNodes();

    curve.move(1, 5.5, 0.2);
    QCOMPARE(curve.nodes().at(1), (Node { 5.5, 0.2 }));

    curve.move(1, 20, 0.2);
    QCOMPARE(curve.nodes().at(1).line, 8 - AutomationCurve::minNodeGap);

    curve.move(1, -20, 0.2);
    QCOMPARE(curve.nodes().at(1).line, AutomationCurve::minNodeGap);
}

void AutomationCurveTest::test_setSpan_shrink_shouldDropOutsideAndKeepTheSound()
{
    auto curve = AutomationCurve { { { 0, 0 }, { 2, 1 }, { 4, 0 }, { 6, 1 }, { 8, 0 } } };
    const auto before = curve;

    curve.setSpan(3, 7);

    QCOMPARE(curve.nodes(), (AutomationCurve::NodeList { { 3, 0.5 }, { 4, 0 }, { 6, 1 }, { 7, 0.5 } }));
    for (double line = 3; line <= 7; line += 0.25) {
        QCOMPARE(curve.valueAt(line), before.valueAt(line));
    }
}

void AutomationCurveTest::test_setSpan_grow_shouldHoldTheEnds()
{
    auto curve = threeNodes();

    curve.setSpan(0, 12);

    QCOMPARE(curve.nodes(), (AutomationCurve::NodeList { { 0, 0 }, { 4, 1 }, { 8, 0.5 }, { 12, 0.5 } }));
}

void AutomationCurveTest::test_snapLine_shouldRoundOnlyWhenSnapping()
{
    QCOMPARE(AutomationCurve::snapLine(3.4, true), 3.0);
    QCOMPARE(AutomationCurve::snapLine(3.6, true), 4.0);
    QCOMPARE(AutomationCurve::snapLine(3.4, false), 3.4);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutomationCurveTest)
