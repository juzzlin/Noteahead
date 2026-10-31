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

#ifndef NOTE_COLUMN_RENDERER_TEST_HPP
#define NOTE_COLUMN_RENDERER_TEST_HPP

#include <QObject>

namespace noteahead {

class NoteColumnRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void test_valueRuns_allUnset_shouldFindNothing();
    void test_valueRuns_allSet_shouldFindOneRun();
    void test_valueRuns_singleValue_shouldFindRunOfOne();
    void test_valueRuns_singleValueAtStart_shouldFindRunOfOne();
    void test_valueRuns_singleValueAtEnd_shouldFindRunOfOne();
    void test_valueRuns_gap_shouldSplitIntoRuns();
    void test_valueRuns_empty_shouldFindNothing();

    void test_paint_singleLineAutomation_shouldDrawMarkOnItsRow();
    void test_paint_multiLineAutomation_shouldDrawTrace();

    void test_automationCurveAt_onTrace_shouldFindAutomation();
    void test_automationCurveAt_awayFromTrace_shouldFindNothing();
    void test_automationCurveAt_tintMode_shouldFindNothing();
    void test_setHoverPosition_onTrace_shouldHoverCurve();
    void test_setHoverPosition_awayFromTrace_shouldClearHover();
    void test_setAutomationDisplayMode_tint_shouldClearHover();
};

} // namespace noteahead

#endif // NOTE_COLUMN_RENDERER_TEST_HPP
