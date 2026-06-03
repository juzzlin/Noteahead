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

#ifndef SYNTH_TEST_HPP
#define SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class SynthTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_defaultValues_shouldBeCorrect();
    void test_parameterSetting_shouldUpdateValues();
    void test_polyphony_shouldActiveMultipleVoices();
    void test_presets_shouldLoadCorrectValues();
    void test_midiCc_shouldUpdateParameters();
    void test_presetMidiCcReset_shouldRestorePresetValues();
    void test_lfoModulation_shouldUpdateInternalState();
    void test_voiceStealing_shouldStealQuietestVoice();
    void test_softClipper_shouldPreventClipping();
    void test_reset_shouldRestoreDefaults();
    void test_portamento_shouldGlideFrequency();
    void test_portamentoOff_shouldJumpImmediately();
    void test_parameterDiscreteFlag_shouldReturnCorrectDiscreteState();
    void test_midiBankAndProgramChange_shouldLoadCorrectPreset();
    void test_userPresets_shouldSaveAndLoad();
    void test_userPresetsDiscreteValues_shouldLoadCorrectly();
    void test_projectLoadPhaseSync_shouldLoadCorrectly();
    void test_serialization_shouldSaveAndLoadGain();
    void test_midiCcResetPanAndVolume_shouldRestoreManualValues();
    void test_projectLoadMidiCcReset_shouldRestoreLoadedValues();
    void test_adsrEnvelope_shouldUpdateStepsOnSampleRateChange();
    void test_pitchBend_shouldUpdateFrequency();
    void test_pulseWidth_shouldUpdateDutyCycle();
    void test_pwm_shouldModulatePulseWidth();
    void test_midiVelocity_shouldAffectVolume();
    void test_velocitySensitivity_shouldAffectVoiceVelocity();
    void test_oscillatorOptimization_shouldSkipSilentOscillators();
};

} // namespace noteahead

#endif // SYNTH_TEST_HPP
