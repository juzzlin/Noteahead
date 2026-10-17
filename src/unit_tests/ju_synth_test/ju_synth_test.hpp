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

#ifndef JU_SYNTH_TEST_HPP
#define JU_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class JuSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_noteOn_default_shouldPlayTheNotePitch();
    void test_range_16Feet_shouldPlayAnOctaveDown();
    void test_subLevel_shouldAddAnOctaveBelow();
    void test_pulseWidth_narrowed_shouldAddEvenHarmonics();
    void test_lpfCutoff_lowered_shouldDarkenTheTone();
    void test_lpfResonance_shouldBoostAroundTheCutoff();
    void test_hpfCutoff_raised_shouldThinTheBass();
    void test_chorusMode_off_shouldBeMono();
    void test_chorusMode_on_shouldSpreadTheSides();
    void test_voices_seventhNote_shouldStealTheOldest();
    void test_panSpread_zero_shouldBeMono();
    void test_panSpread_full_shouldAlternateSides();
    void test_panSpread_full_shouldKeepTheChordBalanced();
    void test_vcaMode_gate_shouldIgnoreTheEnvelope();
    void test_settings_shouldRoundTrip();
    void test_presets_shouldAllLoadAndBeNamedOnce();
    void test_programChange_shouldLoadThePreset();
    void test_automatableParameters_shouldOfferTheMainControls();
    void test_advanceIdle_shouldKeepTheLfoRunning();
};

} // namespace noteahead

#endif // JU_SYNTH_TEST_HPP
