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

#ifndef DRUM_SYNTH_909_TEST_HPP
#define DRUM_SYNTH_909_TEST_HPP

#include <QObject>

namespace noteahead {

class DrumSynth909Test : public QObject
{
    Q_OBJECT

private slots:
    void test_pads_shouldHaveTheRd9Knobs();
    void test_everyPad_shouldSoundAndThenStop();
    void test_everyPad_shouldPeakAtItsTargetUnderTheKick();
    void test_loadKit_shouldMixAsNamed();
    void test_tom_tune_shouldFollowTheRd9();
    void test_tom_decay_shouldMatchTheRd9();
    void test_tom_resonance_shouldLiftTheLowTomsThirdHarmonic();
    void test_snare_tune_shouldFollowTheRd9();
    void test_snare_tone_shouldHoldTheNoiseLonger();
    void test_snare_snappy_shouldTradeBodyForNoise();
    void test_hat_tune_shouldPlayTheSpectrumFaster();
    void test_hat_decay_shouldMatchTheRd9();
    void test_cymbal_tune_shouldShortenTheSample();
    void test_reverseCrash_shouldEndOnTheStrike();
    void test_xmlSerialization_shouldRestoreTheSound();
};

} // namespace noteahead

#endif // DRUM_SYNTH_909_TEST_HPP
