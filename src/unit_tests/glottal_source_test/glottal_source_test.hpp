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

#ifndef GLOTTAL_SOURCE_TEST_HPP
#define GLOTTAL_SOURCE_TEST_HPP

#include <QObject>

namespace noteahead {

class GlottalSourceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_pulse_shouldRestBetweenPeriods();
    void test_pulse_shouldRepeatAtTheFundamental();
    void test_openQuotient_shouldSetHowLongTheFoldsAreOpen_data();
    void test_openQuotient_shouldSetHowLongTheFoldsAreOpen();
    void test_openQuotient_shouldChangeTheHarmonicBalance();
    void test_openQuotient_shouldNotChangeTheExcitationLevel();
    void test_openQuotient_shouldBeClampedToWhatAFoldDoes();

    void test_openness_shouldAverageOneOverAPeriod_data();
    void test_openness_shouldAverageOneOverAPeriod();
    void test_openness_shouldFollowThePulse();

    void test_jitter_shouldVaryThePeriod();
    void test_jitter_shouldHoldWithinAPeriod();
    void test_shimmer_shouldVaryThePulseHeight();
    void test_noPerturbation_shouldBePerfectlyPeriodic();

    void test_sawModel_shouldMatchThePlainOscillator();
    void test_sawModel_openness_shouldBeFlat();

    void test_reset_shouldRepeatTheSameOutput();
};

} // namespace noteahead

#endif // GLOTTAL_SOURCE_TEST_HPP
