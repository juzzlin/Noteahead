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

#ifndef COMPRESSOR_TEST_HPP
#define COMPRESSOR_TEST_HPP

#include <QObject>

namespace noteahead {

class CompressorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_gain_belowThreshold_shouldPassThroughUnchanged();
    void test_gain_aboveThreshold_shouldSettleToRatioReduction();
    void test_gain_higherRatio_shouldReduceMore();
    void test_gain_makeup_shouldLiftOutput();

    void test_knee_zero_shouldNotReduceAtThreshold();
    void test_knee_soft_shouldReduceBelowThreshold();

    void test_attack_slow_shouldReachReductionLater();
    void test_release_afterSignalStops_shouldReturnTowardsUnity();

    void test_lookahead_shouldDelayOutputButNotDetection();
    void test_presets_shouldMapToTheirAuthoredValues();
    void test_presets_glue_shouldReduceGentlyAtProgramLevel();
    void test_presets_names_shouldRoundTrip();

    void test_detectorMode_default_shouldBePeak();
    void test_detectorMode_rms_shouldIgnoreShortTransients();
    void test_detectorMode_rms_shouldSettleToSameReduction();

    void test_reset_afterCompressing_shouldReturnToUnity();
    void test_sidechainSourceDeviceIndex_unset_shouldBeEmpty();
};

} // namespace noteahead

#endif // COMPRESSOR_TEST_HPP
