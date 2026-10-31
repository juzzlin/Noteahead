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

#ifndef AUTOMATION_CURVE_TEST_HPP
#define AUTOMATION_CURVE_TEST_HPP

#include <QObject>

namespace noteahead {

class AutomationCurveTest : public QObject
{
    Q_OBJECT

private slots:
    void test_constructor_unsortedNodes_shouldSortByLine();
    void test_ramp_shouldHaveTwoNodesAtTheEnds();
    void test_ramp_zeroLength_shouldHaveOneNode();
    void test_valueAt_betweenNodes_shouldBeLinear();
    void test_valueAt_outside_shouldHoldTheEnds();
    void test_valueAt_empty_shouldBeZero();
    void test_insert_betweenNodes_shouldKeepOrder();
    void test_insert_offLine_shouldKeepTheLine();
    void test_insert_atOrPastEnds_shouldRefuse();
    void test_insert_tooCloseToNode_shouldRefuse();
    void test_insert_onCurve_shouldNotChangeTheSound();
    void test_remove_end_shouldRefuse();
    void test_remove_inner_shouldRemove();
    void test_move_end_shouldOnlyChangeValue();
    void test_move_inner_shouldStopAtNeighbours();
    void test_setSpan_shrink_shouldDropOutsideAndKeepTheSound();
    void test_setSpan_grow_shouldHoldTheEnds();
    void test_snapLine_shouldRoundOnlyWhenSnapping();
    void test_syncDrawnCurve_switchedToDrawn_shouldStartFromTheRamp();
    void test_syncDrawnCurve_switchedAway_shouldDropTheNodes();
    void test_syncDrawnCurve_newStartValue_shouldMoveTheStartNode();
    void test_syncDrawnCurve_newLines_shouldRefitAndTakeTheEnds();
    void test_syncDrawnCurve_newNodes_shouldRoundTheEndsIntoTheValues();
    void test_curveShape_drawn_shouldBeTheNodes();
    void test_curveShape_preset_shouldFollowTheCurve();
};

} // namespace noteahead

#endif // AUTOMATION_CURVE_TEST_HPP
