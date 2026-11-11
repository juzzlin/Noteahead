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

#ifndef SYNTH_D_TEST_HPP
#define SYNTH_D_TEST_HPP

#include <QObject>

namespace noteahead {

class SynthDTest : public QObject
{
    Q_OBJECT

private slots:
    void test_initPreset_heldNote_shouldPlayAtConcertPitch();
    void test_range_feet16_shouldPlayAnOctaveDown();
    void test_osc3_keyboardControlOff_shouldIgnoreTheKey();
    void test_timeKnobs_panelMarks_shouldMatchThePanel();
    void test_notePriority_low_shouldKeepTheLowestKey();
    void test_notePriority_last_shouldFollowTheNewestKey();
    void test_multiTrigger_off_shouldNotRestartTheContours();
    void test_multiTrigger_on_shouldRestartTheContours();
    void test_decaySwitch_off_shouldStopAtOnce();
    void test_decaySwitch_on_shouldRingForTheDecayTime();
    void test_emphasis_full_shouldSingWithEverySourceOff();
    void test_mixer_threeLoudOscillators_shouldLightTheOverloadLed();
    void test_mixer_oneOscillator_shouldLeaveTheOverloadLedDark();
    void test_lfo_idle_shouldKeepTheLedRunning();
    void test_lfo_filterModulation_shouldSweepTheCutoff();
    void test_feedback_full_shouldStayBounded();
    void test_panSpread_zero_shouldBeMono();
    void test_panSpread_full_shouldPutOsc1Left();
    void test_presets_all_shouldPlayAtSimilarLevels();
    void test_presets_all_shouldStayFinite();
};

} // namespace noteahead

#endif // SYNTH_D_TEST_HPP
