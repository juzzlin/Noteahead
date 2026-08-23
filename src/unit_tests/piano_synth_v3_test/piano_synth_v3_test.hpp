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

#ifndef PIANO_SYNTH_V3_TEST_HPP
#define PIANO_SYNTH_V3_TEST_HPP

#include <QObject>

namespace noteahead {

class PianoSynthV3Test : public QObject
{
    Q_OBJECT

private slots:
    void test_shortNote_shouldNotLoseTheStrike();
    void test_attack_shouldShapeTheOnset();
    void test_attack_default_shouldBeNeutral();
    void test_midiNoteOn_shouldActivateAudio();
    void test_midiNoteOff_shouldDecayToSilence();
    void test_polyphony_shouldSupportMultipleSimultaneousNotes();
    void test_sustainPedal_shouldKeepNoteActiveAfterNoteOff();
    void test_sustainPedal_shouldReleaseNoteWhenPedalLifted();
    void test_allNotesOff_shouldSilenceAllVoices();
    void test_serialization_shouldRestoreParameters();
    void test_reset_shouldRestoreFactoryDefaults();
    void test_velocity_shouldFollowSquareLaw();

    // The three corrections V3 exists for. Each is written against V2's behaviour, so each
    // fails if the V3 string is ever made to behave like the older one again.
    void test_velocitySensitivity_shouldFlattenTheLevelWithoutTouchingTone();
    void test_pickupSign_shouldChangePhasesWithoutChangingLevel();
    void test_bichord_shouldLeaveTheLowBassSingleStrung();
    void test_unisonStandIn_shouldNotStepTheSpectrum();
    void test_velocity_shouldBrightenTheStrike();
    void test_brightness_shouldTiltTheStrike();
    void test_brightness_shouldSetHowLongPartialsHold();
    void test_tuning_shouldMatchNoteFrequency_acrossKeyboard();
    void test_tuning_shouldNotDependOnVelocity();
    void test_stretch_shouldSharpenTheTreble_whenEnabled();
    void test_partials_shouldRunSharp_acrossKeyboard();
    void test_partials_shouldDecayFasterThanTheFundamental();
    void test_radiation_shouldLeaveBassFundamentalBelowSecondPartial();
    void test_decay_shouldRunInTwoStages();
    void test_undampedNotes_shouldKeepRinging_afterNoteOff();
    void test_keyboard_shouldSpeak_atEveryRegister();
    void test_keyboard_shouldHoldLevel_acrossRegisters();
    void test_keyboard_shouldFollowReferenceDecay_acrossRegisters();
    void test_keyboard_shouldReportMeasurements_againstReference();
};

} // namespace noteahead

#endif // PIANO_SYNTH_V3_TEST_HPP
