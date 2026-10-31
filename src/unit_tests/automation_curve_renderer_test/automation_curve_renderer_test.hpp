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

#ifndef AUTOMATION_CURVE_RENDERER_TEST_HPP
#define AUTOMATION_CURVE_RENDERER_TEST_HPP

#include <QObject>

namespace noteahead {

class AutomationCurveRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void test_lineAt_xForLine_shouldRoundTrip();
    void test_valueAt_yForValue_shouldRoundTrip();
    void test_lineAt_outsideThePlot_shouldHoldTheEnds();
    void test_nodeAt_nearANode_shouldFindIt();
    void test_nodeAt_awayFromNodes_shouldFindNone();
};

} // namespace noteahead

#endif // AUTOMATION_CURVE_RENDERER_TEST_HPP
