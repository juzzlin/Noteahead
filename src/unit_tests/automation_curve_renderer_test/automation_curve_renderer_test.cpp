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

#include "automation_curve_renderer_test.hpp"

#include "../../application/models/automation_curve_editor.hpp"
#include "../../view/qml/Dialogs/automation_curve_renderer.hpp"

#include <QTest>

#include <memory>

namespace noteahead {

namespace {

std::unique_ptr<AutomationCurveRenderer> renderer()
{
    auto renderer = std::make_unique<AutomationCurveRenderer>();
    renderer->setWidth(420);
    renderer->setHeight(220);
    renderer->setLine0(4);
    renderer->setLine1(20);
    renderer->setMinValue(-100);
    renderer->setMaxValue(100);
    renderer->setNodes(AutomationCurveEditor::toVariantList(AutomationCurve { { { 4, 0 }, { 10, 50 }, { 20, -100 } } }));
    return renderer;
}

} // namespace

void AutomationCurveRendererTest::test_lineAt_xForLine_shouldRoundTrip()
{
    const auto curve = renderer();
    for (const double line : { 4.0, 7.25, 12.0, 20.0 }) {
        QVERIFY(qFuzzyCompare(curve->lineAt(curve->xForLine(line)), line));
    }
    QCOMPARE(curve->xForLine(4), curve->plotRect().left());
    QCOMPARE(curve->xForLine(20), curve->plotRect().right());
}

void AutomationCurveRendererTest::test_valueAt_yForValue_shouldRoundTrip()
{
    const auto curve = renderer();
    for (const double value : { -100.0, -12.5, 30.0, 100.0 }) {
        QVERIFY(qFuzzyCompare(1.0 + curve->valueAt(curve->yForValue(value)), 1.0 + value));
    }
    // Up is more
    QVERIFY(curve->yForValue(100) < curve->yForValue(-100));
}

void AutomationCurveRendererTest::test_lineAt_outsideThePlot_shouldHoldTheEnds()
{
    const auto curve = renderer();

    QCOMPARE(curve->lineAt(-50), 4.0);
    QCOMPARE(curve->lineAt(1000), 20.0);
    QCOMPARE(curve->valueAt(-50), 100.0);
    QCOMPARE(curve->valueAt(1000), -100.0);
}

void AutomationCurveRendererTest::test_nodeAt_nearANode_shouldFindIt()
{
    const auto curve = renderer();

    QCOMPARE(curve->nodeAt(curve->xForLine(10) + 3, curve->yForValue(50) - 3), 1);
    QCOMPARE(curve->nodeAt(curve->xForLine(20), curve->yForValue(-100)), 2);
}

void AutomationCurveRendererTest::test_nodeAt_awayFromNodes_shouldFindNone()
{
    const auto curve = renderer();

    QCOMPARE(curve->nodeAt(curve->xForLine(14), curve->yForValue(0)), -1);
    QCOMPARE(curve->nodeAt(curve->xForLine(10) + 20, curve->yForValue(50)), -1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutomationCurveRendererTest)
