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

#ifndef LFO_TEST_HPP
#define LFO_TEST_HPP

#include <QObject>

namespace noteahead {

class LfoTest : public QObject
{
    Q_OBJECT

private slots:
    void test_waveformNames_shouldContainFiveEntries();

    void test_sineWaveform_shouldOutputZeroAtPhaseZero();
    void test_sineWaveform_shouldOutputOneAtQuarterCycle();
    void test_sawWaveform_shouldStartAtNegativeOne();
    void test_sawWaveform_shouldRampToPositiveOne();
    void test_triangleWaveform_shouldStartAtNegativeOne();
    void test_triangleWaveform_shouldPeakAtMidCycle();
    void test_squareWaveform_shouldOutputOneInFirstHalf();
    void test_squareWaveform_shouldOutputNegativeOneInSecondHalf();

    void test_oneShotMode_shouldStopAfterOneCycle();
    void test_normalMode_shouldContinueAcrossMultipleCycles();

    void test_setPhase_shouldAffectOutput();
    void test_higherFrequency_shouldProduceShorterCycle();

    void test_randomWaveform_shouldProduceValuesInRange();
    void test_randomWaveform_shouldHoldValueForOneCycle();
    void test_randomWaveform_shouldBeDeterministicAfterReset();

    void test_setFrequency_bpm_shouldProduceCycleMatchingBpm();
    void test_setFrequency_bpm_higherBpm_shouldProduceShorterCycle();
    void test_setFrequency_bpm_differentSyncRate_shouldScaleCycle();
};

} // namespace noteahead

#endif // LFO_TEST_HPP
