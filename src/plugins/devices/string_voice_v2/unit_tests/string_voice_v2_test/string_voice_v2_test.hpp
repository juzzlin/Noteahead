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

#ifndef STRING_VOICE_V2_TEST_HPP
#define STRING_VOICE_V2_TEST_HPP

#include <QObject>

namespace noteahead {

class StringVoiceV2Test : public QObject
{
    Q_OBJECT

private slots:
    void test_midiNoteOn_shouldActivateAudio();
    void test_midiNoteOff_shouldDecayToSilence();
    void test_polyphony_shouldSupportMultipleSimultaneousNotes();
    void test_polyphony_shouldNotClipWithManyNotes();
    void test_allNotesOff_shouldSilenceAllVoices();
    void test_serialization_shouldRestoreParameters();
    void test_audio_output_not_zero();
    void test_realtimeCallbacks_shouldNotBeSilencedByEnsemble();
    void test_vibrato_shouldModulatePitchWithinASingleBuffer();
    void test_ensembleMode_shouldSupportChorusIPlusII();
    void test_voiceRegisters_shouldFollowTheKeyboardSplit();
    void test_stringsSwitches_shouldFollowTheKeyboardSplit();
    void test_strings_shouldSoundAtPitchWithALiftedFundamental();
    void test_stringsTone_shouldOpenAndCloseTheTopEnd();
    void test_balance_shouldScaleEachSectionIndependently();
    void test_formants_male_shouldPeakAtOhFrequencies();
    void test_formants_female_shouldNotNotchBetweenPeaks();
    void test_voiceStealing_shouldRemainStableWhenOversubscribed();
    void test_hpfAndLpf_shouldAttenuateSignal();
    void test_polyphony_shouldNotRescaleHeldNotes();
    void test_velocitySensitivity_shouldScaleBothSections();
    void test_panSpread_shouldCreateStereoSeparation();
};

} // namespace noteahead

#endif // STRING_VOICE_V2_TEST_HPP
