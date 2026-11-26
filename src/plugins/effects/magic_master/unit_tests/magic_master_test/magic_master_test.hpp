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

#ifndef MAGIC_MASTER_TEST_HPP
#define MAGIC_MASTER_TEST_HPP

#include <QObject>

namespace noteahead {

class MagicMasterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_allOff_shouldPassAudioThroughUntouched();
    void test_latencyFrames_shouldMatchImpulseResponseForAllCombos();
    void test_subCleanStage_shouldFilterSubsonicAndMonoLowEnd();
    void test_autoEqStage_shouldVisiblyShapeTiltedMixAndBypassCleanly();
    void test_multibandStage_shouldPassQuietSignalAndCompressHotSignal();
    void test_boostingLimiter_shouldHoldFixedBoostAndRespectTruePeakCeiling();
    void test_songContext_shouldAdaptSubHpfAndMonoBassAndBpm();
    void test_calibration_shouldLockEqAndMultibandAndRespectCeiling();
    void test_reset_shouldBeDeterministicWhilePreservingContextAndCalibration();
};

} // namespace noteahead

#endif // MAGIC_MASTER_TEST_HPP
