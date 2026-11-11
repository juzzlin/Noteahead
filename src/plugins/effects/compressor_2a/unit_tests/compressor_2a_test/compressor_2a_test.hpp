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

#ifndef COMPRESSOR_2A_TEST_HPP
#define COMPRESSOR_2A_TEST_HPP

#include <QObject>

namespace noteahead {

class Compressor2ATest : public QObject
{
    Q_OBJECT

private slots:
    void test_peakReduction_zero_shouldPassTheSignal();
    void test_peakReduction_raised_shouldHoldTheLevelDown();

    void test_staticCurve_limit_shouldReduceMoreThanCompress();
    void test_staticCurve_knee_shouldBeSoft();

    void test_attack_shouldPullDownWithinTensOfMilliseconds();
    void test_release_shouldLetHalfGoWithinSixtyMilliseconds();
    void test_release_longBurst_shouldLetGoSlower();

    void test_gain_shouldMoveTheOutput();
    void test_valve_shouldAddASecondHarmonic();
    void test_mix_zero_shouldLeaveTheSignalAlone();

    void test_isSettled_afterSilence_shouldSettle();
};

} // namespace noteahead

#endif // COMPRESSOR_2A_TEST_HPP
