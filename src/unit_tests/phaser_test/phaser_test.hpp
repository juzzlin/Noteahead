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

#ifndef PHASER_TEST_HPP
#define PHASER_TEST_HPP

#include <QObject>

namespace noteahead {

class PhaserTest : public QObject
{
    Q_OBJECT

private slots:
    void test_process_fullWet_shouldPassEverythingAtUnity();
    void test_process_mixedWithDry_shouldNotchTheSpectrum();
    void test_process_moreStages_shouldNotchMoreDeeply();

    void test_depth_zero_shouldHoldTheNotchesStill();
    void test_depth_full_shouldSweepTheNotches();
    void test_lfo_bpmMode_shouldFollowTempo();
    void test_rateDivider_shouldSlowTheSweep();
    void test_rateDivider_bpmMode_shouldSlowTheSweep();

    void test_feedback_full_shouldStayFinite();
    void test_feedback_polarity_shouldChangeTheVoicing();

    void test_stereoPhase_quadrature_shouldOffsetChannels();
    void test_stereoPhase_zero_shouldKeepChannelsIdentical();

    void test_reset_shouldRestartDeterministically();
    void test_mix_zero_shouldPassThroughDry();
};

} // namespace noteahead

#endif // PHASER_TEST_HPP
