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

#ifndef SNARE_SYNTH_TEST_HPP
#define SNARE_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class SnareSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_midiNoteOn_shouldActivateAudio();
    void test_midiNoteOff_shouldNotStopRinging();
    void test_allNotesOff_shouldChokeVoice();
    void test_panSpread_zero_shouldBeStrictlyMono();
    void test_panSpread_full_shouldCreateStereoSeparation();
    void test_keyTrack_shouldFollowPitchOnlyWhenEnabled();
    void test_filters_lpf_and_hpf_shouldAttenuate();
    void test_snareBuzz_shouldAffectSustainRattle();
    void test_click_shouldAffectAttackTransient();
    void test_hardness_shouldAffectTimbreAndOvertones();
    void test_body_shouldAffectShellWeightAndSustain();
    void test_midiCc_shouldReachParameters();
    void test_serialization_shouldRestoreParameters();
    void test_presetInit_shouldResetParameters();
    void test_presets_hit_shouldBeAudibleAndUnclipped();
};

} // namespace noteahead

#endif // SNARE_SYNTH_TEST_HPP
