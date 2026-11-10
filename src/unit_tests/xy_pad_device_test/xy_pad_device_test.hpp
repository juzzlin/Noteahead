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

#ifndef XY_PAD_DEVICE_TEST_HPP
#define XY_PAD_DEVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class XyPadDeviceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_typeId_shouldBeAModulator();
    void test_evaluate_noTarget_shouldSendNothing();
    void test_evaluate_atRest_shouldSendNothing();
    void test_evaluate_press_shouldSlewToTheHand();
    void test_evaluate_release_shouldSpringBackWithinTheReturnTime();
    void test_evaluate_zeroReturnTime_shouldHold();
    void test_evaluate_depth_shouldBeSent();
    void test_evaluate_positionAutomated_shouldMoveThePad();
    void test_evaluate_positionAutomatedWhileHeld_shouldFollowTheHand();
    void test_evaluate_positionAutomatedWhileSpringingBack_shouldSpringBackFirst();
    void test_isHandActive_shouldLastUntilTheSpringBackEnds();
    void test_evaluate_anyBufferSize_shouldStepTheSame();
    void test_setTarget_shouldReleaseTheOldTarget();
    void test_press_outOfRange_shouldClamp();
    void test_xmlSerialization_shouldRestoreTargetsAndControls();
    void test_xmlSerialization_untouched_shouldWriteNoTargets();
    void test_automatableParameters_shouldBeGroupedByPad();
    void test_automatableParameters_position_shouldBeListed();
    void test_engine_padAndLfoOnOneCutoff_shouldAddUp();
    void test_engine_padOnEffect_shouldLetGoAtHome();
    void test_midiCcTarget_key_shouldRoundTripTheAddress();
    void test_midiCcTarget_value_shouldSpanMinToMax();
    void test_midiCcTarget_withRangeOf_shouldKeepTheRangeOnlyBetweenControllers();
    void test_xmlSerialization_midiCcTarget_shouldRestoreTheRange();
    void test_engine_padOnInternalMidiCc_shouldReachTheDeviceThroughItsMappings();
    void test_engine_padOnExternalMidiCc_shouldSendEachValueOnceAndEndAtTheCentre();
    void test_engine_render_shouldSendNothingToExternalGear();
};

} // namespace noteahead

#endif // XY_PAD_DEVICE_TEST_HPP
