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

#ifndef SUB_MIXER_TEST_HPP
#define SUB_MIXER_TEST_HPP

#include <QObject>

namespace noteahead {

class SubMixerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_subMixer_noMembers_shouldStaySilent();
    void test_subMixer_members_shouldSumMemberOutput();
    void test_subMixer_members_shouldNotReachMasterDirectly();
    void test_subMixer_members_shouldRenderBeforeSubMixer();
    void test_subMixer_insertEffects_shouldApplyToWholeGroup();
    void test_subMixer_nested_shouldSumThroughChain();
    void test_subMixer_nonMember_shouldStillReachMasterDirectly();
    void test_subMixer_memberSends_shouldStillReachSendBus();
    void test_subMixer_memberSends_shouldMatchUngroupedSend();

    void test_membership_addedTwice_shouldNotDuplicate();
    void test_membership_secondSubMixer_shouldTakeOverExclusively();
    void test_membership_selfReference_shouldBeRejected();
    void test_membership_cycle_shouldBeRejected();
    void test_membership_indirectCycle_shouldBeRejected();
    void test_membership_clearedDevice_shouldBePruned();
    void test_membership_nonSubMixerTarget_shouldBeRejected();

    void test_midiCc_volume_shouldScaleGroup();
    void test_midiCc_pan_shouldMoveGroup();
    void test_midiCc_resetAllControllers_shouldRestoreManualValues();
    void test_midiCc_availableControllers_shouldOfferVolumeAndPan();
};

} // namespace noteahead

#endif // SUB_MIXER_TEST_HPP
