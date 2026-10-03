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

#ifndef DRUM_SYNTH_808_TEST_HPP
#define DRUM_SYNTH_808_TEST_HPP

#include <QObject>

namespace noteahead {

class DrumSynth808Test : public QObject
{
    Q_OBJECT

private slots:
    void test_pads_shouldHaveTheRd8Knobs();
    void test_voiceName_shouldNameThe808sPads();
    void test_everyPad_shouldSoundAndThenStop();
    void test_everyPad_shouldPeakAtItsTargetUnderTheKick();
    void test_loadKit_shouldMixAsNamed();
    void test_kick_shouldPlayWhatKick808Plays();
    void test_kick_retrigger_shouldNotClick();
    void test_kick_tuning_shouldSpanTheRd8sRange();
    void test_tom_noon_shouldSettleAtTheRd8sPitch();
    void test_tom_tune_shouldSpanTheRd8sRange();
    void test_tom_switched_shouldPlayACongaWithoutNoise();
    void test_rim_switched_shouldPlayTheClaves();
    void test_clap_switched_shouldPlayShortMaracas();
    void test_clap_offset_shouldSetLevelAndLength();
    void test_snare_tone_shouldTradeTheTwoTones();
    void test_snare_snappyZero_shouldLeaveOnlyTheTones();
    void test_cowbell_shouldBeTheRd8sTwoSquares();
    void test_openHat_shouldHoldAndThenClose();
    void test_hats_tone_shouldThinThemOutAboveNoon();
    void test_cymbal_lowBand_shouldSwellAfterTheHit();
    void test_reverseCymbal_shouldEndOnTheHit();
    void test_xmlSerialization_shouldRestoreTheSound();
};

} // namespace noteahead

#endif // DRUM_SYNTH_808_TEST_HPP
