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

#ifndef COMPRESSOR_76_TEST_HPP
#define COMPRESSOR_76_TEST_HPP

#include <QObject>

namespace noteahead {

class Compressor76Test : public QObject
{
    Q_OBJECT

private slots:
    void test_input_minimum_shouldPassWithoutReduction();
    void test_input_raised_shouldCompressDeviceLevel();

    void test_staticCurve_ratios_shouldMatchSlopesAndSmoothKnee();
    void test_stability_extremeFeedback_shouldStayFiniteAndConverge();

    void test_attack_fastVsSlow_shouldClampAtExpectedSpeed();
    void test_attack_allMode_shouldPreserveTransientSnap();
    void test_release_fastVsSlow_andProgramDependence();
    void test_lowCut_shouldKeepSubBassOutOfDetector();

    void test_output_shouldMoveOutputLevel();
    void test_character_blueStripeVsBlackface_shouldAddMoreHarmonicsAndPresence();
    void test_oversampling_shouldMatchLevelAndReduceAliasing();

    void test_mix_zeroAndParallel_andSendMode();
    void test_isSettled_andReset_shouldSettleAndRenderIdentically();
    void test_factoryPresets_shouldHaveUniqueNamesAndRenderFinite();
};

} // namespace noteahead

#endif // COMPRESSOR_76_TEST_HPP
