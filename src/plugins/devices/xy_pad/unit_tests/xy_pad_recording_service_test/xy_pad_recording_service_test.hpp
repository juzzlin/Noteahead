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

#ifndef XY_PAD_RECORDING_SERVICE_TEST_HPP
#define XY_PAD_RECORDING_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class XyPadRecordingServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_take_shouldRecordADrawnCurveOnTheCursorColumn();
    void test_take_shouldBeUndoneInOneStep();
    void test_take_untouchedAxis_shouldRecordNothingForIt();
    void test_take_notArmed_shouldRecordNothing();
    void test_take_overAnAutomation_shouldReplaceTheLinesItCovers();
    void test_setArmed_off_shouldKeepTheTakeSoFar();
};

} // namespace noteahead

#endif // XY_PAD_RECORDING_SERVICE_TEST_HPP
