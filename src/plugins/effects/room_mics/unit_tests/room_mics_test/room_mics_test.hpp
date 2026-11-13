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

#ifndef ROOM_MICS_TEST_HPP
#define ROOM_MICS_TEST_HPP

#include <QObject>

namespace noteahead {

class RoomMicsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_convolver_shouldMatchDirectConvolution();
    void test_convolver_newResponses_shouldTakeOverWithoutAJump();
    void test_designer_closeMicOnTheSource_shouldHearItAtUnity();
    void test_designer_directSound_shouldArriveAfterTheTimeOfFlight();
    void test_designer_figureEight_shouldNotHearItsSide();
    void test_designer_slices_shouldMatchOneGo();
    void test_reverbTimes_absorbentRoom_shouldBeShorter();
    void test_layout_closeMics_shouldFollowTheSource();
    void test_layout_pair_shouldKeepTheRightOnTheRight();
    void test_processBlock_closeMics_shouldHaveNoLatency();
    void test_processBlock_roomPair_shouldHearTheLeftSourceFirstOnTheLeft();
    void test_processBlock_tail_shouldDecayAtTheRoomsReverbTime();
    void test_processBlock_tail_shouldJoinTheEarlyPartWithoutAStep();
    void test_processBlock_levels_shouldMatchBetweenNearAndFarPairs();
    void test_processBlock_live_shouldRedesignWithinAFifthOfASecond();
    void test_isSettled_shouldWaitForTheTail();
    void test_factoryPresets_shouldNameRealParametersAndPlay();
};

} // namespace noteahead

#endif // ROOM_MICS_TEST_HPP
