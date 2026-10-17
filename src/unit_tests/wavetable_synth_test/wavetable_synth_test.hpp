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

#ifndef WAVETABLE_SYNTH_TEST_HPP
#define WAVETABLE_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class WavetableSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_presets_first_shouldBeInit();
    void test_presets_everyValue_shouldNameAParameterInItsRange();
    void test_loadPreset_shouldResetWhatItDoesNotName();
    void test_presets_everyOne_shouldPlayWithoutClipping();
    void test_lfoRateDivider_default_shouldBeOne();
    void test_lfoRateDivider_shouldRoundTrip();
    void test_lfoRateDivider_projectWithoutIt_shouldLoadOne();
    void test_lfoRateDivider_shouldSlowTheVoicesLfo();
    void test_phaseRandomness_default_shouldBeFullyRandom();
    void test_phaseRandomness_shouldRoundTrip();
    void test_phaseRandomness_projectWithoutIt_shouldLoadFullyRandom();
    void test_phaseRandomness_zero_shouldStartEveryNoteTheSame();
    void test_phaseRandomness_zeroDual_shouldStartEveryPairTheSame();
    void test_phaseRandomness_full_shouldVaryFromNoteToNote();
    void test_phaseRandomness_zero_shouldStartAsLoudAsTheRandomAverage();
    void test_phaseRandomness_partial_shouldRenderTheSameEveryTime();

    void test_lpfSlope_shouldDefaultToTheSlopeItAlwaysHad();
    void test_lpfSlope_shallow_shouldKeepMoreOfTheTop();
    void test_name_shouldReturnCorrectName();
    void test_defaultValues_shouldBeCorrect();
    void test_parameterSetting_shouldUpdateValues();
    void test_polyphony_shouldActiveMultipleVoices();
    void test_midiCc_shouldUpdateParameters();
    void test_reset_shouldRestoreDefaults();
    void test_serialization_shouldPreserveState();
    void test_pitchBend_shouldUpdateFrequency();
    void test_audio_shouldProcessWhenActive();
    void test_hpf_shouldUpdateParameterAndFilterAudio();
    void test_wavetableSelection_shouldUpdateWavetable();
    void test_lfo2_defaultValues_shouldBeCorrect();
    void test_lfo2_parameterSetting_shouldUpdateValues();
    void test_lfo2_serialization_shouldPreserveState();
    void test_lfoEngagement_shouldDefaultToImmediate();
    void test_lfoEngagement_parameterSetting_shouldUpdateValues();
    void test_lfoWaveform_random_serialization_shouldPreserveState();
    void test_lfo2Waveform_random_serialization_shouldPreserveState();

    void test_midiCcModWheel_shouldNotMoveTheIntensityKnob();
    void test_midiCcModWheel_atRest_shouldLeaveThePatchAlone();
    void test_resetAllControllers_shouldTakeBackTheModWheel();
    void test_lfoTarget_volume_shouldModulateAmplitude();
    void test_lfoTarget_resonance_shouldModulateResonance();
    void test_lfoTarget_pan_shouldModulatePanning();
    void test_panSpread_poly_shouldAlternateSides();
    void test_panSpread_polyZero_shouldBeMono();
    void test_lfo2Target_volume_shouldModulateAmplitude();
    void test_lfo2Target_resonance_shouldModulateResonance();
    void test_lfo2Target_pan_shouldModulatePanning();

    void test_voiceMode_unison_shouldMatchPolyLevel();
    void test_voiceMode_dual_shouldMatchPolyLevel();
    void test_voiceMode_supersaw_shouldMatchPolyLevel();
    void test_voiceMode_drift_shouldMatchPolyLevel();
    void test_voiceMode_mono_shouldMatchPolyLevel();
    void test_voiceMode_supersaw_shouldSpaceDetuneUnevenly();
    void test_voiceMode_supersaw_zeroDepth_shouldCollapseToOneVoice();
    void test_voiceMode_drift_shouldNotDetuneStatically();
    void test_voiceMode_dual_shouldDetuneVoicePairs();
    void test_voiceMode_mono_overlappingNotes_shouldUseOneVoice();
    void test_voiceMode_serialization_shouldPreserveEveryMode();

    void test_lfoIntensity_shouldApplyTheDepthItReadsOut();

    void test_midiCc_shouldNotChangeAuthoredValue();
    void test_resetAllControllers_shouldRestoreAuthoredValue();

    void test_ampCurve_shouldSteepenTheAudibleDecay();
    void test_curve_serialization_shouldPreserveState();
    void test_ampVelocitySensitivity_default_shouldBeFull();
    void test_ampVelocitySensitivity_shouldScaleHowMuchVelocityMatters();
    void test_ampVelocitySensitivity_serialization_shouldPreserveState();
    void test_ampVelocitySensitivity_missingFromProject_shouldPlayAsBefore();
    void test_repeatedNote_afterVoiceRetired_shouldSweepTheSameWay();

    void test_lfoFreeRun_default_shouldBeOff();
    void test_lfoFreeRun_shouldRoundTrip();
    void test_lfoFreeRun_projectWithoutIt_shouldLoadOff();
    void test_lfoFreeRun_off_shouldStartEveryNoteFromTheTop();
    void test_lfoFreeRun_on_shouldKeepRunningThroughTheSilence();
    void test_lfoFreeRun_idle_shouldKeepTimeAsRenderingDoes();
    void test_lfoLed_noteOn_shouldRestartTheShape();
    void test_lfoLed_freeRun_shouldNotRestartOnTheNote();
    void test_lfoLed_idle_shouldGoDark();
    void test_lfoLed_renderedSilence_shouldGoDark();
    void test_lfoLed_idleFreeRun_shouldKeepShowingTheLfo();
};

} // namespace noteahead

#endif // WAVETABLE_SYNTH_TEST_HPP
