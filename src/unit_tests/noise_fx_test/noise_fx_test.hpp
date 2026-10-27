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

#ifndef NOISE_FX_TEST_HPP
#define NOISE_FX_TEST_HPP

#include <QObject>

namespace noteahead {

class NoiseFxTest : public QObject
{
    Q_OBJECT

private slots:
    void test_noiseFx_noteOn_shouldProduceAudio();
    void test_noiseFx_noteOn_untriggered_shouldBeSilent();
    void test_noiseFx_playToEnd_shouldReleaseAtSweepEnd_data();
    void test_noiseFx_playToEnd_shouldReleaseAtSweepEnd();
    void test_noiseFx_playToEnd_noteOff_shouldBeIgnored();
    void test_noiseFx_playToEnd_off_noteOff_shouldRelease();
    void test_noiseFx_playToEnd_loopingSweep_noteOff_shouldRelease();
    void test_noiseFx_chromatic_shouldScaleTimesWithNote_data();
    void test_noiseFx_chromatic_shouldScaleTimesWithNote();
    void test_noiseFx_chromatic_shouldScaleCutoffWithNote_data();
    void test_noiseFx_chromatic_shouldScaleCutoffWithNote();
    void test_noiseFx_sweepCutoff_shouldOpenFilter();
    void test_noiseFx_sweepLevel_positiveDepth_shouldFadeIn();
    void test_noiseFx_resetAudio_shouldRenderIdentically();
    void test_noiseFx_width_zero_shouldPlaySameNoiseOnBothChannels();
    void test_noiseFx_width_full_shouldDecorrelateChannels();
    void test_noiseFx_tone_shouldPlayNotePitch_data();
    void test_noiseFx_tone_shouldPlayNotePitch();
    void test_noiseFx_sweepPitch_shouldRaisePitch();
    void test_noiseFx_ringMod_shouldKeepToneLevel();
    void test_noiseFx_gate_shouldChopAtDivision();
    void test_noiseFx_lfoLevel_shouldModulateLevel();
    void test_noiseFx_lfoLevel_smoothRandom_shouldNotStep();
    void test_noiseFx_lfoLevel_random_shouldStep();
    void test_noiseFx_noiseLevel_shouldScaleTheNoise();
    void test_noiseFx_noiseLevel_zero_shouldLeaveTheCrackle();
    void test_noiseFx_sweepKey_shouldPrefixSweepIndex();
    void test_noiseFx_sweepParameters_shouldRoundTrip();
    void test_noiseFx_presets_shouldAllLoadAndBeNamedOnce();
    void test_noiseFx_programChange_shouldLoadThePreset();
};

} // namespace noteahead

#endif // NOISE_FX_TEST_HPP
