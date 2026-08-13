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

#ifndef AUTO_FILTER_TEST_HPP
#define AUTO_FILTER_TEST_HPP

#include <QObject>

namespace noteahead {

class AutoFilterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_process_lowPass_shouldAttenuateHighFrequencies();
    void test_process_highPass_shouldAttenuateLowFrequencies();
    void test_process_bandPass_shouldPassItsBandAtUnity();
    void test_process_notch_shouldRejectItsBand();

    void test_cutoffLfo_zeroIntensity_shouldNotSweep();
    void test_cutoffLfo_fullIntensity_shouldSweepCutoff();
    void test_cutoffLfo_negativeIntensity_shouldInvertSweep();
    void test_cutoffLfo_bpmMode_shouldFollowTempo();

    void test_resonanceLfo_fullIntensity_shouldModulateResonance();
    void test_resonanceLfo_fullModulation_shouldStayFinite();

    void test_envelopeFollower_positiveAmount_shouldOpenFilter();
    void test_envelopeFollower_negativeAmount_shouldCloseFilter();
    void test_envelopeFollower_zeroAmount_shouldNotFollowLevel();

    void test_stereoPhase_halfCycle_shouldOffsetChannels();
    void test_stereoPhase_zero_shouldKeepChannelsIdentical();

    void test_reset_shouldRestartDeterministically();
    void test_mix_zero_shouldPassThroughDry();
};

} // namespace noteahead

#endif // AUTO_FILTER_TEST_HPP
