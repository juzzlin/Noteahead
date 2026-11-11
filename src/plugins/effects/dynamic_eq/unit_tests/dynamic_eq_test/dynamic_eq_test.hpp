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

#ifndef DYNAMIC_EQ_TEST_HPP
#define DYNAMIC_EQ_TEST_HPP

#include <QObject>

namespace noteahead {

class DynamicEqTest : public QObject
{
    Q_OBJECT

private slots:
    void test_processBlock_belowThreshold_shouldBeBitTransparent();
    void test_processBlock_loudAtTheBand_shouldCutByRange();
    void test_processBlock_positiveRange_shouldBoost();
    void test_processBlock_rangeCap_shouldHoldEvenWhenFarOver();
    void test_processBlock_bandDetector_shouldIgnoreOtherFrequencies();
    void test_processBlock_wideDetector_shouldHearTheWholeSignal();
    void test_processBlock_flat_shouldMoveTheWholeSignal();
    void test_processBlock_detectorLpf_shouldIgnoreTheTop();
    void test_processBlock_externalKey_shouldDriveTheBand();
    void test_processBlock_sideMode_shouldIgnoreTheMid();
    void test_processBlock_release_shouldReturnToFlat();
    void test_processBlock_listen_shouldPlayTheDetector();
    void test_gainHistory_shouldFollowTheBand();
    void test_detectorLevelDb_shouldReadWhatTheDetectorHears();
    void test_factoryPresets_shouldNameRealParameters();
};

} // namespace noteahead

#endif // DYNAMIC_EQ_TEST_HPP
