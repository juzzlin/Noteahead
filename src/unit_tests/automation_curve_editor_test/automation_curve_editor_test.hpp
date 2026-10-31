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

#ifndef AUTOMATION_CURVE_EDITOR_TEST_HPP
#define AUTOMATION_CURVE_EDITOR_TEST_HPP

#include <QObject>

namespace noteahead {

class AutomationCurveEditorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_edit_noNodes_shouldStartFromTheRamp();
    void test_edit_nodes_shouldKeepThem();
    void test_insertNode_snapped_shouldLandOnALine();
    void test_insertNode_free_shouldKeepTheLine();
    void test_insertNode_onANode_shouldRefuse();
    void test_insertNode_value_shouldKeepToTheRangeAndStep();
    void test_moveNode_end_shouldOnlyMoveTheValue();
    void test_moveNode_snappedAgainstANeighbour_shouldStayOnTheGrid();
    void test_moveNode_snappedBetweenAdjacentNeighbours_shouldStayWhereItIs();
    void test_removeNode_end_shouldRefuse();
    void test_undo_gesture_shouldRestoreTheCurveBeforeIt();
    void test_endGesture_unchanged_shouldNotAddAnUndoStep();
    void test_variantList_shouldRoundTrip();
};

} // namespace noteahead

#endif // AUTOMATION_CURVE_EDITOR_TEST_HPP
