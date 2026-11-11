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

#ifndef PIANO_SYNTH_ACOUSTIC_TEST_HPP
#define PIANO_SYNTH_ACOUSTIC_TEST_HPP

#include <QObject>

namespace noteahead {

class PianoSynthAcousticTest : public QObject
{
    Q_OBJECT

private slots:
    void test_string_strike_shouldSoundAtTheStretchedPitch();
    void test_string_strike_stretchOff_shouldSoundEqualTempered();
    void test_string_damp_shouldDieFasterThanHeld();
    void test_string_damp_undampedNote_shouldKeepRinging();
    void test_device_sustainPedal_shouldHoldReleasedNotes();
    void test_device_resonance_pedalDown_shouldRingTheOtherStrings();
    void test_device_softPedal_shouldSoftenTheStrike();
    void test_device_panSpread_shouldPutTheBassLeftAndTheTrebleRight();
    void test_device_panSpread_zero_shouldCentreEveryKey();
    void test_device_restrike_shouldKeepOneVoicePerStrike();
    void test_keyboard_peaks_shouldFollowTheRecording();
    void test_presets_every_shouldPeakNearInit();
};

} // namespace noteahead

#endif // PIANO_SYNTH_ACOUSTIC_TEST_HPP
