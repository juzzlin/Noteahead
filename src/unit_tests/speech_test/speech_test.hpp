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

#ifndef SPEECH_TEST_HPP
#define SPEECH_TEST_HPP

#include <QObject>

namespace noteahead {

class SpeechTest : public QObject
{
    Q_OBJECT

private slots:
    void test_sequencer_freeMode_shouldUseNaturalDurations();
    void test_sequencer_freeMode_rate_shouldScaleTheWholePhrase();
    void test_sequencer_fitMode_shouldSpanTheGivenLength_data();
    void test_sequencer_fitMode_shouldSpanTheGivenLength();
    void test_sequencer_fitMode_shouldKeepTheRelativeRhythm();
    void test_sequencer_gridMode_shouldGiveEachSyllableOneDivision();
    void test_sequencer_gridMode_shouldStretchTheVowelNotTheConsonants();
    void test_sequencer_anyMode_shouldNeverProduceAnInaudiblePhoneme();

    void test_sequencer_phraseMode_shouldSpeakEverythingFromTheStart();
    void test_sequencer_stepMode_shouldAdvanceOneSyllablePerTrigger();
    void test_sequencer_stepMode_shouldWrapAtTheEndOfThePhrase();
    void test_sequencer_stepMode_heldNote_shouldSustainTheFinalVowel();
    void test_sequencer_emptyPhrase_shouldNeverBecomeActive();

    void test_device_noteOn_shouldProduceAudio();
    void test_device_loudness_shouldMatchTheRestOfTheRack();
    void test_device_velocitySensitivity_shouldScaleTheLevel_data();
    void test_device_velocitySensitivity_shouldScaleTheLevel();
    void test_device_velocitySensitivity_shouldDefaultToHalf();
    void test_device_noteOn_shouldFollowTheNotePitch();
    void test_device_tuning_shouldBeExact_data();
    void test_device_tuning_shouldBeExact();
    void test_device_stressedSyllable_shouldTakeAPitchAccent();
    void test_device_voiceType_shouldRaiseTheFormants();
    void test_device_formantShift_shouldBeNeutralAtHalfTravel();

    void test_device_phrase_shouldCompileOnAssignment();
    void test_device_emptyPhrase_shouldStaySilent();
    void test_device_afterThePhrase_shouldGoInactive();
    void test_device_endOfPhrase_shouldNotStep();
    void test_device_fitMode_shouldFollowTheContextTempo();
    void test_device_midiCc_shouldReachTheParameters_data();
    void test_device_midiCc_shouldReachTheParameters();
    void test_device_midiCc_shouldNotAuthorTheProject();
    void test_device_midiCc_reset_shouldRestoreTheAuthoredValues();

    void test_device_allNotesOff_shouldStopSpeaking();
    void test_device_reset_shouldRestoreTheDefaultPhrase();
};

} // namespace noteahead

#endif // SPEECH_TEST_HPP
