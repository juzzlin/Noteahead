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

#ifndef BASS_SYNTH_TEST_HPP
#define BASS_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class BassSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_serialization_shouldRestoreParameters();
    void test_midiProcessing_shouldTriggerAudio();
    void test_legatoSlide_shouldStayActive();
    void test_velocityAndAccent_shouldTriggerAudio();
    void test_retriggerOnSlide_shouldIncreaseVolume();
    void test_noClickOnSlideZero_shouldNotHaveLargeDiscontinuities();
    void test_outputLevel_shouldBeCorrect();
    void test_noteOff_shouldCutNoteQuickly();
    void test_sineWave_noClickAtAttack();
    void test_midiVelocity_shouldAffectVolume();
    void test_subOscOptimization_shouldSkipSilentSubOsc();
};

} // namespace noteahead

#endif // BASS_SYNTH_TEST_HPP
