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

#ifndef EQ_32_BAND_GRAPHICAL_TEST_HPP
#define EQ_32_BAND_GRAPHICAL_TEST_HPP

#include <QObject>

namespace noteahead {

class Eq32BandGraphicalTest : public QObject
{
    Q_OBJECT

private slots:
    void test_magnitudeDbAt_centres_shouldMatchTheSliders();
    void test_magnitudeDbAt_allUp_shouldStayFlatToTheTop();
    void test_processBlock_flat_shouldBeBitTransparent();
    void test_processBlock_sine_shouldMatchTheCurve();
    void test_processBlock_midMode_shouldLeaveTheSideAlone();
    void test_processBlock_sideMode_shouldLeaveTheMidAlone();
    void test_processBlock_zigzag_shouldStayStable();
    void test_processBlock_lowSampleRate_shouldStayFinite();
    void test_outputGain_shouldScaleTheCurve();
    void test_factoryPresets_flat_shouldComeFirst();
    void test_factoryPresets_slider_shouldLandOnItsBand();
    void test_parameterDisplay_everyBand_shouldBeDescribed();

    void test_linearPhase_off_shouldHaveNoLatency();
    void test_linearPhase_impulse_shouldArriveAtTheLatency();
    void test_linearPhase_impulseResponse_shouldBeSymmetric();
    void test_linearPhase_coldStart_shouldBeReadyWithinTheLatency();
    void test_linearPhase_sine_shouldMatchTheCurve();
    void test_linearPhase_midMode_shouldDelayTheSideUnchanged();
    void test_linearPhase_sliderMoved_shouldReachTheNewCurve();
    void test_linearPhase_sliderMoved_shouldRenderTheSameEveryTime();
    void test_linearPhase_tail_shouldKeepTheEffectUnsettled();
    void test_effectRack_latency_shouldSumTheEnabledEffects();
    void test_autoGain_minimumPhase_shouldLevelPinkNoise();
    void test_autoGain_linearPhase_shouldLevelPinkNoise();
    void test_autoGain_on_shouldMatchWhatIsHeard();
    void test_autoGain_midMode_shouldLeaveTheSideAlone();
    void test_autoGain_off_shouldLeaveOutputUnchanged();
};

} // namespace noteahead

#endif // EQ_32_BAND_GRAPHICAL_TEST_HPP
