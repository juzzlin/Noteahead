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

#include "automation_curve_editor_test.hpp"

#include "../../application/models/automation_curve_editor.hpp"

#include <QTest>

namespace noteahead {

namespace {

using NodeList = AutomationCurve::NodeList;

QVariantList variantNodes(const NodeList & nodes)
{
    return AutomationCurveEditor::toVariantList(AutomationCurve { nodes });
}

} // namespace

void AutomationCurveEditorTest::test_edit_noNodes_shouldStartFromTheRamp()
{
    AutomationCurveEditor editor;
    editor.edit({}, 4, 12, 10, 100, 0, 127, 1);

    QCOMPARE(editor.curve().nodes(), (NodeList { { 4, 10 }, { 12, 100 } }));
    QCOMPARE(editor.line0(), 4ULL);
    QCOMPARE(editor.line1(), 12ULL);
    QVERIFY(!editor.canUndo());
}

void AutomationCurveEditorTest::test_edit_nodes_shouldKeepThem()
{
    AutomationCurveEditor editor;
    editor.edit(variantNodes({ { 0, 0.2 }, { 3.5, 0.9 }, { 8, 0.4 } }), 0, 8, 0, 0, 0, 1, 0);

    QCOMPARE(editor.curve().nodes(), (NodeList { { 0, 0.2 }, { 3.5, 0.9 }, { 8, 0.4 } }));
}

void AutomationCurveEditorTest::test_insertNode_snapped_shouldLandOnALine()
{
    AutomationCurveEditor editor;
    editor.edit({}, 0, 8, 0, 0, 0, 1, 0);

    QCOMPARE(editor.insertNode(2.4, 0.5, true), 1);
    QCOMPARE(editor.curve().nodes().at(1), (AutomationCurve::Node { 2, 0.5 }));
}

void AutomationCurveEditorTest::test_insertNode_free_shouldKeepTheLine()
{
    AutomationCurveEditor editor;
    editor.edit({}, 0, 8, 0, 0, 0, 1, 0);

    QCOMPARE(editor.insertNode(2.4, 0.5, false), 1);
    QCOMPARE(editor.curve().nodes().at(1).line, 2.4);
}

void AutomationCurveEditorTest::test_insertNode_onANode_shouldRefuse()
{
    AutomationCurveEditor editor;
    editor.edit(variantNodes({ { 0, 0 }, { 2, 1 }, { 8, 0 } }), 0, 8, 0, 0, 0, 1, 0);

    QCOMPARE(editor.insertNode(2.2, 0.5, true), -1);
    QCOMPARE(editor.curve().nodes().size(), 3UL);
}

void AutomationCurveEditorTest::test_insertNode_value_shouldKeepToTheRangeAndStep()
{
    AutomationCurveEditor editor;
    editor.edit({}, 0, 8, 0, 0, 0, 127, 1);

    editor.insertNode(2, 200, true);
    editor.insertNode(4, 10.4, true);
    editor.insertNode(6, -3, true);

    QCOMPARE(editor.curve().nodes().at(1).value, 127.0);
    QCOMPARE(editor.curve().nodes().at(2).value, 10.0);
    QCOMPARE(editor.curve().nodes().at(3).value, 0.0);
}

void AutomationCurveEditorTest::test_moveNode_end_shouldOnlyMoveTheValue()
{
    AutomationCurveEditor editor;
    editor.edit({}, 0, 8, 0, 0, -100, 100, 0);

    editor.moveNode(0, 3, 50, true);
    editor.moveNode(1, 5, -25.5, false);

    QCOMPARE(editor.curve().nodes(), (NodeList { { 0, 50 }, { 8, -25.5 } }));
}

void AutomationCurveEditorTest::test_moveNode_snappedAgainstANeighbour_shouldStayOnTheGrid()
{
    AutomationCurveEditor editor;
    editor.edit(variantNodes({ { 0, 0 }, { 2, 1 }, { 3, 0 }, { 8, 0 } }), 0, 8, 0, 0, 0, 1, 0);

    // Dragged onto and past the node on line 3: the last line short of it, not a hair short of it
    editor.moveNode(1, 3.2, 0.5, true);
    QCOMPARE(editor.curve().nodes().at(1).line, 2.0);

    editor.moveNode(1, -5, 0.5, true);
    QCOMPARE(editor.curve().nodes().at(1).line, 1.0);
}

void AutomationCurveEditorTest::test_moveNode_snappedBetweenAdjacentNeighbours_shouldStayWhereItIs()
{
    AutomationCurveEditor editor;
    editor.edit(variantNodes({ { 0, 0 }, { 2, 1 }, { 2.5, 0 }, { 3, 1 }, { 8, 0 } }), 0, 8, 0, 0, 0, 1, 0);

    // No whole line between 2 and 3 to snap to
    editor.moveNode(2, 2.9, 0.75, true);

    QCOMPARE(editor.curve().nodes().at(2), (AutomationCurve::Node { 2.5, 0.75 }));
}

void AutomationCurveEditorTest::test_removeNode_end_shouldRefuse()
{
    AutomationCurveEditor editor;
    editor.edit(variantNodes({ { 0, 0 }, { 2, 1 }, { 8, 0 } }), 0, 8, 0, 0, 0, 1, 0);

    QVERIFY(!editor.removeNode(0));
    QVERIFY(!editor.removeNode(2));
    QVERIFY(!editor.removeNode(-1));
    QVERIFY(editor.removeNode(1));
    QCOMPARE(editor.curve().nodes(), (NodeList { { 0, 0 }, { 8, 0 } }));
}

void AutomationCurveEditorTest::test_undo_gesture_shouldRestoreTheCurveBeforeIt()
{
    AutomationCurveEditor editor;
    editor.edit({}, 0, 8, 0.5, 0.5, 0, 1, 0);
    const auto original = editor.curve();

    // Added and dragged through several places in one press
    editor.beginGesture();
    const int index = editor.insertNode(2, 0.5, true);
    editor.moveNode(index, 3, 0.7, true);
    editor.moveNode(index, 4, 0.9, true);
    editor.endGesture();

    editor.beginGesture();
    editor.moveNode(0, 0, 0.1, true);
    editor.endGesture();

    QVERIFY(editor.canUndo());
    editor.undo();
    QCOMPARE(editor.curve().nodes(), (NodeList { { 0, 0.5 }, { 4, 0.9 }, { 8, 0.5 } }));
    editor.undo();
    QCOMPARE(editor.curve(), original);
    QVERIFY(!editor.canUndo());
}

void AutomationCurveEditorTest::test_endGesture_unchanged_shouldNotAddAnUndoStep()
{
    AutomationCurveEditor editor;
    editor.edit({}, 0, 8, 0.5, 0.5, 0, 1, 0);

    // A press on a node without a drag
    editor.beginGesture();
    editor.moveNode(0, 0, 0.5, true);
    editor.endGesture();

    QVERIFY(!editor.canUndo());
}

void AutomationCurveEditorTest::test_variantList_shouldRoundTrip()
{
    const AutomationCurve curve { { { 0, 0.25 }, { 1.5, 1 }, { 4, 0 } } };

    QCOMPARE(AutomationCurveEditor::fromVariantList(AutomationCurveEditor::toVariantList(curve)), curve);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutomationCurveEditorTest)
