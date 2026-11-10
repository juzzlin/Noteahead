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

#ifndef AUTOMATION_TAKE_TEST_HPP
#define AUTOMATION_TAKE_TEST_HPP

#include <QObject>

namespace noteahead {

class AutomationTakeTest : public QObject
{
    Q_OBJECT

private slots:
    void test_split_newPattern_shouldStartAPiece();
    void test_split_lineGoesBack_shouldStartAPiece();
    void test_simplify_straightLine_shouldKeepTheEnds();
    void test_simplify_corner_shouldKeepIt();
    void test_curve_shouldSpanTheLinesPlayed();
    void test_curve_pastTheEnd_shouldStopAtTheLastLine();
    void test_curve_lastLine_shouldBeNothing();
    void test_remainderOutside_noOverlap_shouldKeepItAsIs();
    void test_remainderOutside_covered_shouldBeNothing();
    void test_remainderOutside_middle_shouldKeepBothSidesAsTheyPlay();
    void test_steps_shouldHoldEachSettingUpToTheNearestLine();
    void test_steps_twiceOnOneLine_shouldKeepTheLast();
    void test_remainderOutside_midiCc_shouldKeepItsValuesInSteps();
};

} // namespace noteahead

#endif // AUTOMATION_TAKE_TEST_HPP
