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

#ifndef STRING_ENSEMBLE_TEST_HPP
#define STRING_ENSEMBLE_TEST_HPP

#include <QObject>

namespace noteahead {

class StringEnsembleTest : public QObject
{
    Q_OBJECT

private slots:
    void test_midiNoteOn_shouldActivateAudio();
    void test_midiNoteOff_shouldDecayToSilence();
    void test_allNotesOff_shouldSilenceAllVoices();
    void test_registers_allDisabledShouldBeSilent();
    void test_registers_violinShouldSoundAnOctaveAboveViola();
    void test_registers_hornShouldSoundAnOctaveBelowViola();
    void test_bassSplit_belowSplitShouldReachBassSectionOnly();
    void test_bassSplit_atOrAboveSplitShouldReachUpperSectionOnly();
    void test_volumeBass_shouldScaleBassSection();
    void test_velocity_shouldScaleLevel();
    void test_velocitySensitivity_zeroShouldIgnoreVelocity();
    void test_crescendo_longAttackShouldRampUp();
    void test_polyphony_shouldNotClipWithManyNotes();
    void test_modulation_shouldCreateStereoSeparation();
    void test_phaser_shouldAlterSignal();
    void test_hpfAndLpf_shouldAttenuateSignal();
    void test_midiCc_shouldUpdateVolumeAndPan();
    void test_serialization_shouldRestoreParameters();
};

} // namespace noteahead

#endif // STRING_ENSEMBLE_TEST_HPP
