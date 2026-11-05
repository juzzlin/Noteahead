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

#ifndef DOUBLER_TEST_HPP
#define DOUBLER_TEST_HPP

#include <QObject>

namespace noteahead {

class DoublerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_monoSum_monoSafe_shouldReturnTheInputMid();

    void test_amount_zero_shouldLeaveTheSignalAlone();
    void test_amount_raised_monoInput_shouldBuildSide();

    void test_haas_impulse_shouldMoveTheRightChannelLate();

    void test_lowCut_shouldKeepLowFrequenciesOutOfTheCopy();
    void test_lowCut_default_sawNotes_shouldStayCentred();

    void test_isSettled_afterSilence_shouldSettle();
};

} // namespace noteahead

#endif // DOUBLER_TEST_HPP
