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

#ifndef DRUM_SYNTH_TEST_HPP
#define DRUM_SYNTH_TEST_HPP

#include <QObject>


namespace noteahead {

class DrumSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_kickEngine_attack_shouldAddClick();
    void test_kickEngine_trigger_shouldBeActive();
    void test_kickEngine_nextSample_shouldEventuallyDeactivate();
    void test_kick_start_discontinuity();
    void test_kick_retrigger_pop();
    void test_kick_small_attack_pop();
    void test_snareEngine_trigger_shouldBeActive();
    void test_hihatEngine_trigger_shouldBeActive();
    void test_crashEngine_trigger_shouldBeActive();
    void test_rideEngine_trigger_shouldBeActive();
    void test_tomEngine_trigger_shouldBeActive();
    void test_clapEngine_trigger_shouldBeActive();
    void test_drumSynthDevice_midiNoteOn_shouldTriggerVoice();
    void test_drumSynthDevice_xmlSerialization_shouldRestoreParameters();
    void test_processMidiCc_shouldUpdateVoicePanLpfHpf();
    void test_drumSynthDevice_toms_shouldHaveDifferentDefaultTunes();
    void test_tomEngine_tunes_shouldSoundDifferent();
};

} // namespace noteahead

#endif // DRUM_SYNTH_TEST_HPP
