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

#ifndef PIANO_SYNTH_TEST_HPP
#define PIANO_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class PianoSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_midiNoteOn_shouldActivateAudio();
    void test_midiNoteOff_shouldDecayToSilence();
    void test_polyphony_shouldSupportMultipleSimultaneousNotes();
    void test_sustainPedal_shouldKeepNoteActiveAfterNoteOff();
    void test_sustainPedal_shouldReleaseNoteWhenPedalLifted();
    void test_allNotesOff_shouldSilenceAllVoices();
    void test_serialization_shouldRestoreParameters();
    void test_velocity_shouldAffectOutputLevel();
};

} // namespace noteahead

#endif // PIANO_SYNTH_TEST_HPP
