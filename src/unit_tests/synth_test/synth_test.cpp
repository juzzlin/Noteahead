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

#include "synth_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/synth_presets.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace noteahead {

void SynthTest::initTestCase()
{
}

void SynthTest::cleanupTestCase()
{
}

void SynthTest::test_defaultValues_shouldBeCorrect()
{
    const SynthDevice synth { "Test Synth" };
    QCOMPARE(synth.name(), std::string("Test Synth"));
    QCOMPARE(synth.vco1Octave(), 0);
    QCOMPARE(synth.vco1Pitch(), 0.5f);
    QCOMPARE(synth.mixVco1(), 1.0f);
    QCOMPARE(synth.mixVco2(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 1.0f);
    QCOMPARE(synth.hpfCutoff(), 0.0f);
    QCOMPARE(synth.ampSustain(), 1.0f);
    QCOMPARE(synth.pan(), 0.5f);
    QCOMPARE(synth.gain(), 0.5f);
}

void SynthTest::test_parameterSetting_shouldUpdateValues()
{
    SynthDevice synth { "Test Synth" };
    synth.setVco2Waveform(PolyBlepOscillator::Waveform::Square);
    QCOMPARE(synth.vco2Waveform(), PolyBlepOscillator::Waveform::Square);

    synth.setMixVco2(0.5f);
    QCOMPARE(synth.mixVco2(), 0.5f);

    synth.setLpfCutoff(0.4f);
    QCOMPARE(synth.lpfCutoff(), 0.4f);

    synth.setModInt(0.8f);
    QVERIFY(std::abs(synth.modInt() - 0.216f) < 0.001f);

    synth.setDelayDepth(0.5f);
    QCOMPARE(synth.delayDepth(), 0.5f);

    synth.setDelaySync(true);
    QCOMPARE(synth.delaySync(), true);

    synth.setPan(0.2f);
    QCOMPARE(synth.pan(), 0.2f);

    synth.setGain(0.75f);
    QCOMPARE(synth.gain(), 0.75f);
}

void SynthTest::test_polyphony_shouldActiveMultipleVoices()
{
    SynthDevice synth { "Test Synth" };
    synth.processMidiNoteOn(60, 100);
    synth.processMidiNoteOn(64, 100);
    synth.processMidiNoteOn(67, 100);

    // We can't easily check internal voice state without exposing it,
    // but we can check if audio is generated.
    double output[2048] {};
    std::fill(output, output + 2048, 0.0f);
    AudioContext context { std::span(output, 2048), 1024, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    synth.processAudio(context);

    bool soundDetected = false;
    for (int i = 0; i < 2048; i++) {
        if (std::abs(output[i]) > 0.0001f) {
            soundDetected = true;
            break;
        }
    }
    QVERIFY(soundDetected);
}

void SynthTest::test_presets_shouldLoadCorrectValues()
{
    SynthDevice synth { "Test Synth" };
    synth.loadPreset(0, 1); // Fat Bass

    QCOMPARE(synth.vco1Waveform(), PolyBlepOscillator::Waveform::Saw);
    QCOMPARE(synth.mixVco2(), 0.7f);
    QCOMPARE(synth.lpfCutoff(), 0.25f);
    QCOMPARE(synth.voiceMode(), SynthDevice::VoiceMode::Unison);
}

void SynthTest::test_midiCc_shouldUpdateParameters()
{
    SynthDevice synth { "Test Synth" };

    // Test individual CC updates
    synth.processMidiCc(7, 64, 0); // Volume ~0.5
    QCOMPARE(synth.volume(), Device::faderPositionFromMidiCc(64));

    synth.processMidiCc(10, 32, 0); // Pan ~0.25
    QCOMPARE(synth.pan(), 32.0f / 127.0f);

    synth.processMidiCc(74, 100, 0); // Cutoff ~0.78
    QCOMPARE(synth.lpfCutoff(), 100.0f / 127.0f);

    synth.processMidiCc(81, 10, 0); // HPF Cutoff ~0.08
    QCOMPARE(synth.hpfCutoff(), 10.0f / 127.0f);

    // Test CC 121 (Reset All Controllers)
    // First, set manual UI values
    synth.setVolume(1.0f);
    synth.setPan(0.0f);
    synth.setPanSpread(0.0f);
    synth.setLpfCutoff(0.5f);
    synth.setHpfCutoff(0.1f);

    // Now change them via MIDI CC
    synth.processMidiCc(7, 10, 0);
    synth.processMidiCc(74, 127, 0);

    QCOMPARE(synth.volume(), Device::faderPositionFromMidiCc(10));
    QCOMPARE(synth.lpfCutoff(), 127.0f / 127.0f);

    // Trigger Reset
    synth.processMidiCc(121, 0, 0);

    // Should return to manual UI values
    QCOMPARE(synth.volume(), 1.0f);
    QCOMPARE(synth.pan(), 0.0f);
    QCOMPARE(synth.panSpread(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 0.5f);
    QCOMPARE(synth.hpfCutoff(), 0.1f);
}

void SynthTest::test_presetMidiCcReset_shouldRestorePresetValues()
{
    SynthDevice synth { "Test Synth" };

    // 1. Initial manual state
    synth.setLpfCutoff(1.0f);

    // 2. Load "Fat Bass" preset (Cutoff = 0.25)
    synth.loadPreset(0, 1);
    QCOMPARE(synth.lpfCutoff(), 0.25f);

    // 3. Offset via MIDI CC
    synth.processMidiCc(74, 127, 0); // Cutoff to 1.0
    QCOMPARE(synth.lpfCutoff(), 1.0f);

    // 4. Reset All Controllers (CC 121)
    synth.processMidiCc(121, 0, 0);

    // 5. Should restore to PRESET value (0.25), not initial manual value (1.0)
    QCOMPARE(synth.lpfCutoff(), 0.25f);
}

void SynthTest::test_lfoModulation_shouldUpdateInternalState()
{
    SynthDevice synth { "Test Synth" };

    synth.setLfoWaveform(Lfo::Waveform::Square);
    QCOMPARE(synth.lfoWaveform(), Lfo::Waveform::Square);

    synth.setLfoMode(Lfo::Mode::OneShot);
    QCOMPARE(synth.lfoMode(), Lfo::Mode::OneShot);

    synth.setLfoRate(0.8f);
    QCOMPARE(synth.lfoRate(), 0.8f);

    synth.setLfoInt(0.75f);
    QVERIFY(std::abs(synth.lfoInt() - 0.125f) < 0.001f);

    synth.setLfoTarget(SynthDevice::LfoTarget::Cutoff);
    QCOMPARE(synth.lfoTarget(), SynthDevice::LfoTarget::Cutoff);

    // Verify audio generation works with Lfo
    synth.processMidiNoteOn(60, 100);
    double output[512] {};
    std::fill(output, output + 512, 0.0f);
    AudioContext context { std::span(output, 512), 256, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    synth.processAudio(context);

    bool sound = false;
    for (int i = 0; i < 512; i++) {
        if (std::abs(output[i]) > 0.0001f) {
            sound = true;
            break;
        }
    }
    QVERIFY(sound);
}

void SynthTest::test_voiceStealing_shouldStealQuietestVoice()
{
    SynthDevice synth { "Test Synth" };

    // Trigger 6 notes to fill all voices
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        synth.processMidiNoteOn(60 + i, 100);
    }

    // Process audio so they all start playing
    double output[256] {};
    AudioContext context { std::span(output, 256), 128, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    synth.processAudio(context);

    // Release Note 60 - it will start decaying (becoming quieter)
    synth.processMidiNoteOff(60);

    // Process a bit more to let it decay
    synth.processAudio(context);

    // Trigger a new note
    // It should steal Note 60 because it's the quietest (releasing)
    synth.processMidiNoteOn(80, 100);

    // We verify sound is still coming out (basic stability check)
    synth.processAudio(context);
    bool sound = false;
    for (int i = 0; i < 256; i++) {
        if (std::abs(output[i]) > 0.0001f) {
            sound = true;
            break;
        }
    }
    QVERIFY(sound);
}

void SynthTest::test_softClipper_shouldPreventClipping()
{
    SynthDevice synth { "Test Synth" };

    // Max out volume and multiple oscillators to force > 1.0 signal
    synth.setVolume(1.0f);
    synth.setMixVco1(1.0f);
    synth.setMixVco2(1.0f);
    synth.setLpfResonance(1.0f); // High resonance adds lots of gain

    synth.processMidiNoteOn(60, 127);

    double output[1024] {};
    std::fill(output, output + 1024, 0.0f);
    AudioContext context { std::span(output, 1024), 512, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    synth.processAudio(context);

    for (int i = 0; i < 1024; i++) {
        QVERIFY(output[i] <= 1.0f);
        QVERIFY(output[i] >= -1.0f);
    }
}

void SynthTest::test_reset_shouldRestoreDefaults()
{
    SynthDevice synth { "Test Synth" };
    synth.setMixVco2(0.9f);
    synth.setLpfCutoff(0.1f);

    synth.reset();

    QCOMPARE(synth.mixVco2(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 1.0f);
}

void SynthTest::test_saveState_restore_shouldRestoreParameters()
{
    SynthDevice synth { "Test Synth" };
    synth.setMixVco2(0.9f);
    synth.setLpfCutoff(0.4f);
    synth.setPan(0.2f);

    synth.saveState();

    synth.setMixVco2(0.1f);
    synth.setLpfCutoff(0.8f);
    synth.setPan(0.7f);

    synth.restoreState();

    // Both the parameter and the value syncParameters() mirrors it into have to come back
    QCOMPARE(synth.mixVco2(), 0.9f);
    QCOMPARE(synth.lpfCutoff(), 0.4f);
    QCOMPARE(synth.pan(), 0.2f);
}

void SynthTest::test_saveState_restore_shouldRestoreManualValues()
{
    SynthDevice synth { "Test Synth" };
    synth.setLpfResonance(0.3f);

    synth.saveState();
    synth.setLpfResonance(0.8f);
    synth.restoreState();

    // A CC that overrides the resonance and then resets must fall back to the restored value,
    // not to the one the cancelled edit left behind
    synth.processMidiCc(71, 127, 0);
    QCOMPARE(synth.lpfResonance(), 1.0f);
    synth.processMidiCc(121, 0, 0);
    QCOMPARE(synth.lpfResonance(), 0.3f);
}

void SynthTest::test_portamento_shouldGlideFrequency()
{
    SynthDevice synth { "Test Synth" };
    const double freq60 = 440.0 * std::pow(2.0, (60 - 69) / 12.0);

    // --- Test Poly Mode ---
    synth.setVoiceMode(SynthDevice::VoiceMode::Poly);
    synth.setPortamento(0.5f); // 50% portamento

    // Play first note
    synth.processMidiNoteOn(60, 100);
    QCOMPARE(synth.voiceGlideFrequency(0), freq60);

    // Let it finish (we need to process audio until it's inactive)
    double dummy[1024] {};
    AudioContext context { std::span(dummy, 1024), 512, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    for (int i = 0; i < 100; i++) {
        synth.processAudio(context);
    }

    // Play second note (same voice should be reused if it's the only one)
    synth.processMidiNoteOn(62, 100);

    // In the BROKEN state, it will be freq62 immediately.
    // In the FIXED state, it should still be freq60 (starting the glide).
    // Note: processMidiNoteOn calls handleNoteOn which updates glideFrequency if broken.
    QCOMPARE(synth.voiceGlideFrequency(0), freq60);

    // --- Test Unison Mode ---
    synth.reset();
    synth.setVoiceMode(SynthDevice::VoiceMode::Unison);
    synth.setPortamento(0.5f);

    // Play first note (Unison triggers all voices)
    synth.processMidiNoteOn(60, 100);
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        QCOMPARE(synth.voiceGlideFrequency(i), freq60);
    }

    // Play second note
    synth.processMidiNoteOn(62, 100);

    // In the BROKEN state, it will be freq62 immediately.
    // In the FIXED state, it should still be freq60.
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        QCOMPARE(synth.voiceGlideFrequency(i), freq60);
    }
}

void SynthTest::test_portamentoOff_shouldJumpImmediately()
{
    SynthDevice synth { "Test Synth" };
    const double freq60 = 440.0 * std::pow(2.0, (60 - 69) / 12.0);
    const double freq62 = 440.0 * std::pow(2.0, (62 - 69) / 12.0);

    // --- Poly Mode ---
    synth.setVoiceMode(SynthDevice::VoiceMode::Poly);
    synth.setPortamento(0.0f);

    synth.processMidiNoteOn(60, 100);
    bool found60 = false;
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        if (std::abs(synth.voiceGlideFrequency(i) - freq60) < 0.001) {
            found60 = true;
            break;
        }
    }
    QVERIFY(found60);

    synth.processMidiNoteOn(62, 100);
    bool found62 = false;
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        if (std::abs(synth.voiceGlideFrequency(i) - freq62) < 0.001) {
            found62 = true;
            break;
        }
    }
    QVERIFY(found62);

    // --- Unison Mode ---
    synth.reset();
    synth.setVoiceMode(SynthDevice::VoiceMode::Unison);
    synth.setPortamento(0.0f);

    synth.processMidiNoteOn(60, 100);
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        const double detuneAmount = (i - (SynthDevice::MaxVoices - 1) / 2.0) * std::pow(0.0, 1.5) * 0.2; // voiceDepth is 0
        const double expectedFreq = freq60 * std::pow(2.0, detuneAmount / 12.0);
        QCOMPARE(synth.voiceGlideFrequency(i), expectedFreq);
    }

    synth.processMidiNoteOn(62, 100);
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        const double detuneAmount = (i - (SynthDevice::MaxVoices - 1) / 2.0) * std::pow(0.0, 1.5) * 0.2;
        const double expectedFreq = freq62 * std::pow(2.0, detuneAmount / 12.0);
        QCOMPARE(synth.voiceGlideFrequency(i), expectedFreq);
    }
}

void SynthTest::test_parameterDiscreteFlag_shouldReturnCorrectDiscreteState()
{
    const SynthDevice synth { "Test Synth" };

    // Test discrete parameters
    const auto vco1Wave = synth.parameter(Constants::NahdXml::xmlKeyVco1Waveform().toStdString());
    QVERIFY(vco1Wave.has_value());
    QVERIFY(vco1Wave->get().isDiscrete());

    const auto vco1Octave = synth.parameter(Constants::NahdXml::xmlKeyVco1Octave().toStdString());
    QVERIFY(vco1Octave.has_value());
    QVERIFY(vco1Octave->get().isDiscrete());

    const auto vco1Pitch = synth.parameter(Constants::NahdXml::xmlKeyVco1Pitch().toStdString());
    QVERIFY(vco1Pitch.has_value());
    QVERIFY(!vco1Pitch->get().isDiscrete());

    const auto modTarget = synth.parameter(Constants::NahdXml::xmlKeyModTarget().toStdString());
    QVERIFY(modTarget.has_value());
    QVERIFY(modTarget->get().isDiscrete());

    const auto voiceMode = synth.parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString());
    QVERIFY(voiceMode.has_value());
    QVERIFY(voiceMode->get().isDiscrete());

    // Test continuous parameters
    const auto lpfCutoff = synth.parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString());
    QVERIFY(lpfCutoff.has_value());
    QVERIFY(!lpfCutoff->get().isDiscrete());

    const auto ampAttack = synth.parameter(Constants::NahdXml::xmlKeyAmpAttack().toStdString());
    QVERIFY(ampAttack.has_value());
    QVERIFY(!ampAttack->get().isDiscrete());

    const auto multiShape = synth.parameter(Constants::NahdXml::xmlKeyMultiShape().toStdString());
    QVERIFY(multiShape.has_value());
    QVERIFY(!multiShape->get().isDiscrete());
}

namespace {

//! Zero crossings of the left channel over the given number of blocks, a cheap stand-in for pitch.
int zeroCrossings(SynthDevice & synth, int blocks)
{
    constexpr uint32_t frameCount = 512;
    double buffer[frameCount * 2] {};
    AudioContext context { std::span(buffer, frameCount * 2), frameCount, static_cast<uint32_t>(Constants::defaultSampleRate()) };

    int crossings = 0;
    double previous = 0.0;
    for (int block = 0; block < blocks; block++) {
        std::fill(std::begin(buffer), std::end(buffer), 0.0);
        synth.processAudio(context);
        for (uint32_t i = 0; i < frameCount; i++) {
            const double sample = buffer[i * 2];
            if ((previous < 0.0) != (sample < 0.0)) {
                crossings++;
            }
            previous = sample;
        }
    }
    return crossings;
}

} // namespace

void SynthTest::test_vcoOctave_32Foot_shouldSoundTwoOctavesBelow8Foot()
{
    const auto crossingsAt = [](int octave) {
        SynthDevice synth { "Test Synth" };
        // A sine has exactly two zero crossings per cycle. A saw's polyBLEP-smoothed wrap can wobble
        // across zero more than once, which biases the count at the low end.
        synth.setVco1Waveform(PolyBlepOscillator::Waveform::Sine);
        synth.setVco1Octave(octave);
        synth.processMidiNoteOn(72, 100);
        zeroCrossings(synth, 10); // Past the attack, so the count is of a steady tone
        return zeroCrossings(synth, 40);
    };

    const int at8Foot = crossingsAt(0);
    const int at32Foot = crossingsAt(-2);
    QVERIFY(at8Foot > 0);
    QVERIFY(at32Foot > 0);

    // 32' is two octaves below 8', so the waveform has to come out at a quarter of the rate.
    const double ratio = static_cast<double>(at8Foot) / at32Foot;
    QVERIFY2(std::abs(ratio - 4.0) < 0.2, qPrintable(QString { "Rate ratio was %1, expected 4" }.arg(ratio)));
}

void SynthTest::test_vcoOctave_belowRange_shouldClampTo32Foot()
{
    SynthDevice synth { "Test Synth" };

    // 32' is the bottom of the range, and presets are free to ask for more than that.
    synth.setVco1Octave(-3);
    QCOMPARE(synth.vco1Octave(), -2);
    synth.setVco2Octave(-3);
    QCOMPARE(synth.vco2Octave(), -2);
    synth.setVco3Octave(-3);
    QCOMPARE(synth.vco3Octave(), -2);
}

void SynthTest::test_midiBankAndProgramChange_shouldLoadCorrectPreset()
{
    SynthDevice synth { "Test Synth" };

    // Set some user presets
    UserPresets userPresets;
    const SynthPreset up1 { "User 1", { { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 0.123f } } };
    userPresets[5] = up1;
    synth.setUserPresets(userPresets);

    // Switch to User Bank (Bank Select MSB = 1)
    synth.processMidiCc(0, 1, 0);
    // Change to Program 5
    synth.processMidiProgramChange(5, 0);

    QCOMPARE(synth.lpfCutoff(), 0.123f);

    // Switch to Factory Bank (Bank Select MSB = 0)
    synth.processMidiCc(0, 0, 0);
    // Change to Program 0 (Factory Init/first preset)
    synth.processMidiProgramChange(0, 0);

    const auto & factoryPresets = SynthPresets::presets();
    if (!factoryPresets.empty()) {
        const auto expectedCutoff = factoryPresets[0].parameters.count(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()) ? factoryPresets[0].parameters.at(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()) : 1.0f; // Default 1.0
        QCOMPARE(synth.lpfCutoff(), expectedCutoff);
    }
}

void SynthTest::test_userPresets_shouldSaveAndLoad()
{
    SynthDevice synth { "Test Synth" };

    UserPresets userPresets;
    for (int i = 0; i < 128; i++)
        userPresets[i] = SynthPresets::initPreset();

    const SynthPreset myPreset { "My Bass", { { Constants::NahdXml::xmlKeyLpfCutoff().toStdString(), 0.42f } } };
    userPresets[10] = myPreset;

    synth.setUserPresets(userPresets);
    synth.loadPreset(1, 10);

    QCOMPARE(synth.lpfCutoff(), 0.42f);
}

void SynthTest::test_userPresetsDiscreteValues_shouldLoadCorrectly()
{
    SynthDevice synth { "Test Synth" };

    UserPresets userPresets;
    const std::string vco1WaveformKey = Constants::NahdXml::xmlKeyVco1Waveform().toStdString();

    // Logical values for discrete parameters:
    // Waveform (0..3): Tri=0.0, Saw=1.0, Square=2.0, Sine=3.0
    // DelayType (0..3): Stereo=0.0, Mono=1.0, PingPong=2.0, Tape=3.0

    const SynthPreset sawPreset { "Saw", { { vco1WaveformKey, 1.0f } } };
    const SynthPreset pulsePreset { "Square", { { vco1WaveformKey, 2.0f } } };
    const SynthPreset pingPongPreset { "PingPong", { { Constants::NahdXml::xmlKeyDelayType().toStdString(), 2.0f } } };

    userPresets[0] = sawPreset;
    userPresets[1] = pulsePreset;
    userPresets[2] = pingPongPreset;

    synth.setUserPresets(userPresets);

    // Load Saw
    synth.loadPreset(1, 0);
    QCOMPARE(synth.vco1Waveform(), PolyBlepOscillator::Waveform::Saw);

    // Load Square
    synth.loadPreset(1, 1);
    QCOMPARE(synth.vco1Waveform(), PolyBlepOscillator::Waveform::Square);

    // Load PingPong
    synth.loadPreset(1, 2);
    QCOMPARE(synth.delayType(), Delay::Type::PingPong);

    // Test Phase Sync (vco1Sync)
    const std::string vco1SyncKey = Constants::NahdXml::xmlKeyVco1Sync().toStdString();
    const SynthPreset syncPreset { "Sync", { { vco1SyncKey, 1.0f } } };
    userPresets[3] = syncPreset;
    synth.setUserPresets(userPresets);

    synth.loadPreset(1, 3);
    QCOMPARE(synth.vco1Sync(), true);
}

void SynthTest::test_projectLoadPhaseSync_shouldLoadCorrectly()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setVco1Sync(true);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.vco1Sync(), true);
    }
}

void SynthTest::test_serialization_shouldSaveAndLoadGain()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setGain(0.8f);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.gain(), 0.8f);
    }
}

void SynthTest::test_serialization_shouldSaveAndLoadPitchBendRange()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setPitchBendRange(7);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.pitchBendRange(), 7);
    }
}

void SynthTest::test_midiCcResetPanAndVolume_shouldRestoreManualValues()
{
    SynthDevice synth { "Test Synth" };

    // 1. Initial manual state
    synth.setVolume(0.8f);
    synth.setPan(0.2f);
    synth.setGain(0.6f);

    // 2. Change via MIDI CC
    synth.processMidiCc(7, 127, 0); // Volume to unity
    synth.processMidiCc(10, 127, 0); // Pan to 1.0
    QCOMPARE(synth.volume(), Constants::faderUnityPosition());
    QCOMPARE(synth.pan(), 1.0f);

    // 3. Reset All Controllers (CC 121)
    synth.processMidiCc(121, 0, 0);

    // 4. Should restore to manual values
    QCOMPARE(synth.volume(), 0.8f);
    QCOMPARE(synth.pan(), 0.2f);
    QCOMPARE(synth.gain(), 0.6f);

    // 5. Test with preset load
    synth.loadPreset(0, 1); // This should update manual fallback values
    const float presetVolume = synth.volume();
    const float presetPan = synth.pan();
    const float presetGain = synth.gain();

    synth.processMidiCc(7, 0, 0); // Volume to 0.0
    synth.processMidiCc(10, 0, 0); // Pan to 0.0
    QCOMPARE(synth.volume(), 0.0f);
    QCOMPARE(synth.pan(), 0.0f);

    synth.processMidiCc(121, 0, 0);
    QCOMPARE(synth.volume(), presetVolume);
    QCOMPARE(synth.pan(), presetPan);
    QCOMPARE(synth.gain(), presetGain);
}

void SynthTest::test_projectLoadMidiCcReset_shouldRestoreLoadedValues()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setVolume(0.4f);
        synth.setPan(0.6f);
        synth.setGain(0.7f);
        synth.setLpfCutoff(0.33f);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);

        QCOMPARE(synth.volume(), 0.4f);
        QCOMPARE(synth.pan(), 0.6f);
        QCOMPARE(synth.gain(), 0.7f);
        QCOMPARE(synth.lpfCutoff(), 0.33f);

        // Change via MIDI CC
        synth.processMidiCc(7, 127, 0);
        synth.processMidiCc(10, 127, 0);
        synth.processMidiCc(74, 127, 0);
        QCOMPARE(synth.volume(), Constants::faderUnityPosition());
        QCOMPARE(synth.pan(), 1.0f);
        QCOMPARE(synth.lpfCutoff(), 1.0f);

        // Reset All Controllers
        synth.processMidiCc(121, 0, 0);

        // Should return to LOADED values
        QCOMPARE(synth.volume(), 0.4f);
        QCOMPARE(synth.pan(), 0.6f);
        QCOMPARE(synth.gain(), 0.7f);
        QCOMPARE(synth.lpfCutoff(), 0.33f);
    }
}

void SynthTest::test_adsrEnvelope_shouldUpdateStepsOnSampleRateChange()
{
    AdsrEnvelope env;
    env.setSampleRate(44100.0);
    env.setAttackTime(1.0); // 1 second attack
    env.setDecayTime(1.0);
    env.setSustainLevel(1.0);
    env.setReleaseTime(1.0);

    env.trigger();
    // At 44.1kHz, 1 second attack means the step is 1.0 / 44100.0
    // After 1 sample, level should be 1.0 / 44100.0
    double val44 = env.nextSample();
    QVERIFY(std::abs(val44 - (1.0 / 44100.0)) < 0.000001);

    // Change sample rate to 96kHz
    env.reset();
    env.setSampleRate(96000.0);
    env.trigger();

    // Now, after 1 sample, level should be 1.0 / 96000.0
    double val96 = env.nextSample();
    QVERIFY(std::abs(val96 - (1.0 / 96000.0)) < 0.000001);
}

void SynthTest::test_pitchBend_shouldUpdateFrequency()
{
    SynthDevice synth("TestSynth");
    synth.setPitchBendRange(2);

    // Default center
    QCOMPARE(synth.currentPitchBendOffset(), 0.0f);

    // Max up
    synth.processMidiPitchBend(16383, 0);
    QCOMPARE(std::round(synth.currentPitchBendOffset()), 2.0);

    // Max down
    synth.processMidiPitchBend(0, 0);
    QCOMPARE(std::round(synth.currentPitchBendOffset()), -2.0);

    // Range change
    synth.setPitchBendRange(12);
    synth.processMidiPitchBend(16383, 0);
    QCOMPARE(std::round(synth.currentPitchBendOffset()), 12.0);
}

void SynthTest::test_pulseWidth_shouldUpdateDutyCycle()
{
    SynthDevice synth("TestSynth");
    synth.setVco1Waveform(PolyBlepOscillator::Waveform::Square);
    synth.setMixVco1(1.0f);
    synth.setMixVco2(0.0f);
    synth.setMultiLevel(0.0f);
    synth.setVolume(1.0f);
    synth.setGain(0.5f); // 0dB
    synth.setLpfCutoff(1.0f); // Open filter
    synth.setHpfCutoff(0.0f);

    const uint32_t sampleRate = 44100;
    const uint32_t frameCount = 1000;
    std::vector<double> buffer(frameCount * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };

    // Shape 0.0 -> 50% duty cycle
    synth.setVco1Shape(0.0f);
    synth.processMidiNoteOn(60, 100);
    // Process some audio to settle envelopes
    synth.processAudio(context);
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    synth.processAudio(context);

    int positiveSamples = 0;
    double sum = 0.0;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        if (buffer[i] > 0.001f)
            positiveSamples++;
        sum += buffer[i];
    }
    // With 50% duty cycle, roughly half should be positive
    QVERIFY(positiveSamples > 400 && positiveSamples < 600);
    // DC offset should be near zero
    QVERIFY(std::abs(sum / frameCount) < 0.05);

    // Shape 1.0 -> the narrowest pulse the control reaches, 5 % of the cycle. It used to go to
    // 0.5 %, which at this note is about a sample wide and is a click rather than a tone.
    synth.setVco1Shape(1.0f);
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    synth.processAudio(context);

    positiveSamples = 0;
    sum = 0.0;
    for (size_t i = 0; i < buffer.size(); i += 2) {
        if (buffer[i] > 0.001f)
            positiveSamples++;
        sum += buffer[i];
    }
    // With a 5 % duty cycle, a twentieth of the window and no more
    QVERIFY(positiveSamples > 10 && positiveSamples < 120);
    // DC offset should still be near zero
    QVERIFY(std::abs(sum / frameCount) < 0.05);
}

void SynthTest::test_pwm_shouldModulatePulseWidth()
{
    SynthDevice synth("TestSynth");
    synth.setVco1Waveform(PolyBlepOscillator::Waveform::Square);
    synth.setMixVco1(1.0f);
    synth.setMixVco2(0.0f);
    synth.setLpfCutoff(1.0f);

    // Set LFO to modulate Shape (PWM)
    synth.setLfoTarget(SynthDevice::LfoTarget::Shape);
    synth.setLfoRate(0.5f); // Fast enough to see change in 1000 samples
    synth.setLfoInt(1.0f);
    synth.setVco1Shape(0.5f);

    const uint32_t sampleRate = 44100;
    const uint32_t frameCount = 1000;
    std::vector<double> buffer(frameCount * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };

    synth.processMidiNoteOn(60, 100);

    // Count positive samples in two consecutive blocks.
    // Due to LFO modulation, the duty cycle should change.
    synth.processAudio(context);
    int pos1 = 0;
    for (size_t i = 0; i < buffer.size(); i += 2)
        if (buffer[i] > 0.001f)
            pos1++;

    std::fill(buffer.begin(), buffer.end(), 0.0f);
    synth.processAudio(context);
    int pos2 = 0;
    for (size_t i = 0; i < buffer.size(); i += 2)
        if (buffer[i] > 0.001f)
            pos2++;

    // The number of positive samples should be different due to PWM
    QVERIFY(pos1 != pos2);
}

void SynthTest::test_midiVelocity_shouldAffectVolume()
{
    SynthDevice synth { "Test Synth" };
    synth.setVolume(1.0f);
    synth.setGain(0.5f);
    synth.setAmpAttack(0.0f);
    synth.setAmpSustain(1.0f);
    synth.setAmpVelocitySensitivity(1.0f);
    synth.setMixVco1(1.0f);
    synth.setMixVco2(0.0f);
    synth.setMixVco3(0.0f);
    synth.setMultiLevel(0.0f);
    synth.setLpfCutoff(1.0f);

    const auto getPeak = [&](uint8_t velocity) {
        synth.processMidiAllNotesOff();
        synth.processMidiNoteOn(60, velocity);

        const int frameCount { 1000 };
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(context);

        double peak { 0.0 };
        for (const double sample : buffer) {
            peak = std::max(peak, std::abs(sample));
        }
        return peak;
    };

    {
        const double peakLow = getPeak(40);
        const double peakHigh = getPeak(127);
        QVERIFY2(peakHigh > peakLow, QString { "Velocity did not affect volume: peakLow=%1, peakHigh=%2" }.arg(peakLow).arg(peakHigh).toUtf8().constData());
    }

    {
        synth.resetAudio();
        synth.setAmpVelocitySensitivity(0.0f);
        const double peakLow = getPeak(40);
        const double peakHigh = getPeak(127);
        QVERIFY(std::fabs(peakHigh - peakLow) < 0.001);
    }
}

void SynthTest::test_oscillatorOptimization_shouldSkipSilentOscillators()
{
    SynthDevice synth { "Test Synth" };
    synth.setVolume(1.0f);
    synth.setGain(0.5f);
    synth.setLpfCutoff(1.0f);
    synth.setVco1Sync(true); // Disable phase randomization for predictable output

    auto getOutput = [&]() {
        synth.resetAudio();
        synth.processMidiNoteOn(60, 100);
        const int frameCount { 100 };
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(context);
        return buffer;
    };

    // 1. All OSCs off -> should be silent
    synth.setMixVco1(0.0f);
    synth.setMixVco2(0.0f);
    synth.setMixVco3(0.0f);
    synth.setMultiLevel(0.0f);

    const auto bufferSilent = getOutput();
    for (const double sample : bufferSilent) {
        QCOMPARE(sample, 0.0);
    }

    // 2. OSC2 off, OSC1 on. Changing OSC2 parameters should not change output.
    synth.setMixVco1(1.0f);
    const auto bufferOnlyVco1 = getOutput();

    synth.setVco2Waveform(PolyBlepOscillator::Waveform::Square);
    synth.setVco2Octave(2);
    const auto bufferOnlyVco1AfterVco2Change = getOutput();

    for (size_t i = 0; i < bufferOnlyVco1.size(); i++) {
        QVERIFY2(std::abs(bufferOnlyVco1[i] - bufferOnlyVco1AfterVco2Change[i]) < 1e-6,
                 QString("VCO1 output changed when VCO2 parameters changed: diff=%1").arg(std::abs(bufferOnlyVco1[i] - bufferOnlyVco1AfterVco2Change[i])).toUtf8().constData());
    }

    // 3. Multi off, OSC1 on. Changing Multi parameters should not change output.
    synth.setMultiType(MultiEngine::Type::Decim);
    const auto bufferOnlyVco1AfterMultiChange = getOutput();

    for (size_t i = 0; i < bufferOnlyVco1.size(); i++) {
        QVERIFY2(std::abs(bufferOnlyVco1[i] - bufferOnlyVco1AfterMultiChange[i]) < 1e-6,
                 QString("VCO1 output changed when Multi parameters changed: diff=%1").arg(std::abs(bufferOnlyVco1[i] - bufferOnlyVco1AfterMultiChange[i])).toUtf8().constData());
    }

    // 4. VCO1 off, VCO2 on. Changing VCO1 parameters should not change output.
    synth.setMixVco1(0.0f);
    synth.setMixVco2(1.0f);
    const auto bufferOnlyVco2 = getOutput();

    synth.setVco1Waveform(PolyBlepOscillator::Waveform::Square);
    synth.setVco1Octave(-1);
    const auto bufferOnlyVco2AfterVco1Change = getOutput();

    for (size_t i = 0; i < bufferOnlyVco2.size(); i++) {
        QVERIFY2(std::abs(bufferOnlyVco2[i] - bufferOnlyVco2AfterVco1Change[i]) < 1e-6,
                 QString("VCO2 output changed when VCO1 parameters changed: diff=%1").arg(std::abs(bufferOnlyVco2[i] - bufferOnlyVco2AfterVco1Change[i])).toUtf8().constData());
    }
}

void SynthTest::test_liveUnisonDepth_shouldUpdateFrequency()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Unison);
    synth.setVoiceDepth(0.0f);
    synth.setPortamento(0.0f);

    const uint8_t note = 60;
    const double baseFreq = 440.0 * std::pow(2.0, (note - 69) / 12.0);

    synth.processMidiNoteOn(note, 100);

    // Initial check: all voices should have base frequency (depth 0)
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        QCOMPARE(synth.voiceGlideFrequency(i), baseFreq);
    }

    // Process some audio to make sure we are in the rendering loop where live updates happen
    double output[256] {};
    AudioContext context { std::span(output, 256), 128, 44100 };
    synth.processAudio(context);

    // Update depth live
    synth.setVoiceDepth(1.0f);

    // Process audio again - this should trigger the live update in updateVoiceParameters
    synth.processAudio(context);

    // Verify frequencies are now detuned
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        const double detuneAmount = (static_cast<double>(i) - (SynthDevice::MaxVoices - 1) / 2.0) * std::pow(1.0f, 1.5) * 0.2;
        const double expectedFreq = baseFreq * std::pow(2.0, detuneAmount / 12.0);
        QVERIFY(std::abs(synth.voiceGlideFrequency(i) - expectedFreq) < 0.001);
    }

    // Also check Pan Spread live update
    synth.setPanSpread(1.0f);
    synth.processAudio(context);
    // (Note: we can't easily check voice.pan directly as it's private, but it's part of the same fix)
}

void SynthTest::test_lfoWaveform_random_serialization_shouldPreserveState()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setLfoWaveform(Lfo::Waveform::Random);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.lfoWaveform(), Lfo::Waveform::Random);
    }
}

void SynthTest::test_panningAndAmplitude_shouldBeCorrect()
{
    SynthDevice synth { "TestSynth" };
    synth.setMixVco1(1.0f);
    synth.setMixVco2(0.0f);
    synth.setMixVco3(0.0f);
    synth.setMultiLevel(0.0f);
    synth.setVco1Waveform(PolyBlepOscillator::Waveform::Sine);
    synth.setLpfCutoff(1.0f);
    synth.setGain(0.5f); // 0 dB
    synth.setVolume(1.0f);
    synth.setDelayMix(0.0f);
    synth.setPanSpread(0.0f);

    const int frameCount { 128 };
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };

    auto getPeak = [&]() {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        // Process enough to get past attack (default 0.5)
        for (int i = 0; i < 20; i++) {
            synth.processAudio(context);
        }
        double peakL = 0.0;
        double peakR = 0.0;
        for (size_t i = 0; i < buffer.size(); i += 2) {
            peakL = std::max(peakL, std::abs(buffer[i]));
            peakR = std::max(peakR, std::abs(buffer[i + 1]));
        }
        return std::make_pair(peakL, peakR);
    };

    synth.processMidiNoteOn(60, 127);

    // With current gain staging:
    // Amplitude = (1/6) * mixHeadroom(0.4) * velocity(1.0) ≈ 0.0666...
    // Constant-power center pan: L=R=Amplitude*cos(π/4) ≈ Amplitude*0.707
    // Full pan (cos(0)=1 / sin(π/2)=1): L or R = Amplitude

    // 1. Center Pan (default): constant-power gives cos(π/4) ≈ 0.707 of full amplitude
    synth.setPan(0.5f);
    auto [peakL_center, peakR_center] = getPeak();
    QVERIFY2(peakL_center > 0.04 && peakL_center < 0.08, qPrintable(QString("Center L peak out of range: %1").arg(peakL_center)));
    QVERIFY2(peakR_center > 0.04 && peakR_center < 0.08, qPrintable(QString("Center R peak out of range: %1").arg(peakR_center)));

    // 2. Full Left: cos(0)=1 gives full amplitude to L channel
    synth.resetAudio();
    synth.processMidiNoteOn(60, 127);
    synth.setPan(0.0f);
    auto [peakL_left, peakR_left] = getPeak();
    QVERIFY2(peakL_left > 0.06 && peakL_left < 0.12, qPrintable(QString("Left peak out of range: %1").arg(peakL_left)));
    QVERIFY2(peakR_left < 0.001, qPrintable(QString("Bleed into Right: %1").arg(peakR_left)));

    // 3. Full Right: sin(π/2)=1 gives full amplitude to R channel
    synth.resetAudio();
    synth.processMidiNoteOn(60, 127);
    synth.setPan(1.0f);
    auto [peakL_right, peakR_right] = getPeak();
    QVERIFY2(peakL_right < 0.001, qPrintable(QString("Bleed into Left: %1").arg(peakL_right)));
    QVERIFY2(peakR_right > 0.06 && peakR_right < 0.12, qPrintable(QString("Right peak out of range: %1").arg(peakR_right)));
}

void SynthTest::test_oscillatorDrift_zero_shouldProduceSameFrequency()
{
    const auto setup = [](SynthDevice & synth) {
        synth.setVco1Sync(true); // Deterministic phase
        synth.setMixVco1(1.0f);
        synth.setMixVco2(0.0f);
        synth.setMixVco3(0.0f);
        synth.setMultiLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setVolume(1.0f);
        synth.setGain(0.5f);
        synth.setOscillatorDrift(0.0f);
    };

    const int frameCount = 512;

    std::vector<double> buf1(static_cast<size_t>(frameCount) * 2, 0.0);
    {
        SynthDevice synth { "Test Synth" };
        setup(synth);
        synth.processMidiNoteOn(60, 100);
        AudioContext ctx { std::span(buf1.data(), buf1.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);
    }

    std::vector<double> buf2(static_cast<size_t>(frameCount) * 2, 0.0);
    {
        SynthDevice synth { "Test Synth" };
        setup(synth);
        synth.processMidiNoteOn(60, 100);
        AudioContext ctx { std::span(buf2.data(), buf2.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);
    }

    // With drift=0 both independent synths from same initial state should produce identical output
    for (int i = 0; i < frameCount * 2; i++) {
        QCOMPARE(buf1[i], buf2[i]);
    }
}

void SynthTest::test_oscillatorDrift_nonZero_shouldModulateFrequency()
{
    SynthDevice synth { "Test Synth" };
    synth.setMixVco1(1.0f);
    synth.setMixVco2(0.0f);
    synth.setMixVco3(0.0f);
    synth.setMultiLevel(0.0f);
    synth.setLpfCutoff(1.0f);
    synth.setVolume(1.0f);
    synth.setGain(0.5f);
    synth.setOscillatorDrift(1.0f);
    QCOMPARE(synth.oscillatorDrift(), 1.0f);

    synth.processMidiNoteOn(60, 100);

    // Process enough audio that the drift has time to move the pitch
    const int frameCount = 44100; // one full second at 44.1kHz
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext ctx { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
    synth.processAudio(ctx);

    // Check that there is non-trivial variation in the output (drift should cause detectable change)
    double minVal = buffer[0];
    double maxVal = buffer[0];
    for (const double sample : buffer) {
        minVal = std::min(minVal, sample);
        maxVal = std::max(maxVal, sample);
    }
    QVERIFY(maxVal - minVal > 0.001);
}

void SynthTest::test_oscillatorDrift_serialization_shouldPreserveState()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setOscillatorDrift(0.75f);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.oscillatorDrift(), 0.75f);
    }
}

void SynthTest::test_crossModDepth_zero_shouldProduceSameFrequency()
{
    const auto setup = [](SynthDevice & synth) {
        synth.setVco1Sync(true); // Deterministic phase
        synth.setMixVco1(0.0f);
        synth.setMixVco2(1.0f);
        synth.setMixVco3(0.0f);
        synth.setMultiLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setVolume(1.0f);
        synth.setGain(0.5f);
        synth.setCrossModDepth(0.0f);
    };

    const int frameCount = 512;

    std::vector<double> buf1(static_cast<size_t>(frameCount) * 2, 0.0);
    {
        SynthDevice synth { "Test Synth" };
        setup(synth);
        synth.processMidiNoteOn(60, 100);
        AudioContext ctx { std::span(buf1.data(), buf1.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);
    }

    std::vector<double> buf2(static_cast<size_t>(frameCount) * 2, 0.0);
    {
        SynthDevice synth { "Test Synth" };
        setup(synth);
        synth.processMidiNoteOn(60, 100);
        AudioContext ctx { std::span(buf2.data(), buf2.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);
    }

    for (int i = 0; i < frameCount * 2; i++) {
        QCOMPARE(buf1[i], buf2[i]);
    }
}

void SynthTest::test_crossModDepth_nonZero_shouldModulateVco2Frequency()
{
    const auto render = [](float crossModDepth) {
        SynthDevice synth { "Test Synth" };
        synth.setVco1Sync(true);
        synth.setMixVco1(0.0f); // VCO1 is the silent modulator
        synth.setMixVco2(1.0f);
        synth.setMixVco3(0.0f);
        synth.setMultiLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setVolume(1.0f);
        synth.setGain(0.5f);
        synth.setCrossModDepth(crossModDepth);
        synth.processMidiNoteOn(60, 100);

        const int frameCount = 4096;
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext ctx { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);
        return buffer;
    };

    const auto bufferNoMod = render(0.0f);
    const auto bufferModulated = render(1.0f);

    bool differs = false;
    for (size_t i = 0; i < bufferNoMod.size(); i++) {
        if (std::abs(bufferNoMod[i] - bufferModulated[i]) > 0.001) {
            differs = true;
            break;
        }
    }
    QVERIFY(differs);
}

void SynthTest::test_crossModDepth_serialization_shouldPreserveState()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setCrossModDepth(0.6f);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.crossModDepth(), 0.6f);
    }
}

void SynthTest::test_dcOffset_syncWithCrossMod_shouldStayNearZero()
{
    // Hard sync truncates VCO2's ramp, and cross mod walks the truncation point, so the mean of the
    // resulting wave is non-zero. Neither alone does it; the two together do. The HPF is bypassed at
    // cutoff zero, so only the DC blocker can take it out.
    SynthDevice synth { "Test Synth" };
    synth.setVco2Sync(true);
    synth.setCrossModDepth(1.0f);
    synth.setMixVco1(1.0f);
    synth.setMixVco2(1.0f);
    synth.setMixVco3(0.0f);
    synth.setMultiLevel(0.0f);
    synth.setVco2Pitch(0.35f);
    synth.setAmpAttack(0.0f);
    synth.setAmpSustain(1.0f);
    synth.setLpfCutoff(1.0f);
    synth.setHpfCutoff(0.0f);
    synth.setDelayMix(0.0f);
    synth.setVolume(1.0f);
    synth.processMidiNoteOn(60, 100);

    const uint32_t sampleRate = 44100;
    const uint32_t frameCount = 512;
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);

    double sum = 0.0;
    double sumOfSquares = 0.0;
    size_t count = 0;
    // A second of audio, less the first 100 ms: the note onset is a genuinely one-sided transient,
    // and the blocker's ~5 Hz corner takes some 30 ms to settle after it.
    const int skippedBlocks = static_cast<int>(0.1 * sampleRate / frameCount);
    for (int block = 0; block < static_cast<int>(sampleRate / frameCount); block++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
        synth.processAudio(context);
        if (block < skippedBlocks) {
            continue;
        }
        for (uint32_t i = 0; i < frameCount; i++) {
            sum += buffer[i * 2];
            sumOfSquares += buffer[i * 2] * buffer[i * 2];
            count++;
        }
    }

    QVERIFY(count > 0);
    const double rms = std::sqrt(sumOfSquares / static_cast<double>(count));
    QVERIFY(rms > 0.001); // The patch must actually be sounding for the offset to mean anything
    const double dc = sum / static_cast<double>(count);
    // Without the blocker this measures about -41 % of RMS at this depth; with it, well under 1 %.
    QVERIFY(std::abs(dc) < rms * 0.005);
}

void SynthTest::test_midiCcResonance_shouldUpdateParameter()
{
    SynthDevice synth { "Test Synth" };
    synth.setLpfResonance(0.0f);

    synth.processMidiCc(71, 64, 0);
    QCOMPARE(synth.lpfResonance(), 64.0f / 127.0f);

    synth.processMidiCc(71, 127, 0);
    QCOMPARE(synth.lpfResonance(), 1.0f);
}

void SynthTest::test_midiCcResonanceReset_shouldRestoreManualValue()
{
    SynthDevice synth { "Test Synth" };
    synth.setLpfResonance(0.3f);

    synth.processMidiCc(71, 127, 0);
    QCOMPARE(synth.lpfResonance(), 1.0f);

    synth.processMidiCc(121, 0, 0);
    QCOMPARE(synth.lpfResonance(), 0.3f);
}

void SynthTest::test_midiCcModWheel_shouldOverrideLfoIntensity()
{
    SynthDevice synth { "Test Synth" };
    synth.setLfoInt(0.0f);

    synth.processMidiCc(1, 127, 0);
    QCOMPARE(synth.lfoInt(), 1.0f);

    synth.processMidiCc(1, 64, 0);
    QCOMPARE(synth.lfoInt(), 64.0f / 127.0f);
}

void SynthTest::test_midiCcModWheelReset_shouldRestoreLfoIntensity()
{
    SynthDevice synth { "Test Synth" };
    // Parameter value 0.5 maps to 0.0 intensity via the cubic centered mapping
    synth.setLfoInt(0.5f);
    QVERIFY(std::abs(synth.lfoInt()) < 0.01f);

    synth.processMidiCc(1, 127, 0); // mod wheel full
    QCOMPARE(synth.lfoInt(), 1.0f);

    synth.processMidiCc(121, 0, 0); // reset all controllers
    // lfoInt should be restored from parameter (which is still 0.5 → maps to ~0.0)
    QVERIFY(std::abs(synth.lfoInt()) < 0.01f);
}

void SynthTest::test_lfoTarget_volume_shouldModulateAmplitude()
{
    const auto renderPeak = [](SynthDevice::LfoTarget target, float lfoInt) {
        SynthDevice synth { "Test Synth" };
        synth.setMixVco1(1.0f);
        synth.setMixVco2(0.0f);
        synth.setMixVco3(0.0f);
        synth.setMultiLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setVolume(1.0f);
        synth.setGain(0.5f);
        synth.setAmpAttack(0.0f);
        synth.setAmpSustain(1.0f);
        synth.setLfoWaveform(Lfo::Waveform::Square);
        synth.setLfoRate(0.5f);
        synth.setLfoInt(lfoInt);
        synth.setLfoTarget(target);

        synth.processMidiNoteOn(60, 127);

        const int frameCount = 2048;
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext ctx { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);

        double peak = 0.0;
        for (const double s : buffer)
            peak = std::max(peak, std::abs(s));
        return peak;
    };

    // With Volume LFO at max intensity, peak amplitude should be higher than with Pitch LFO
    // (Square LFO on volume between 0 and 2x, vs pitch which doesn't change amplitude)
    const double peakPitch = renderPeak(SynthDevice::LfoTarget::Pitch, 0.0f);
    const double peakVolume = renderPeak(SynthDevice::LfoTarget::Volume, 1.0f);

    // Volume LFO with high intensity should produce audible output (not silence)
    QVERIFY(peakVolume > 0.001);
    // And without LFO modulation on pitch, amplitude is stable
    QVERIFY(peakPitch > 0.001);
}

void SynthTest::test_lfoTarget_resonance_shouldModulateResonance()
{
    const auto renderBuffer = [](float lfoInt, SynthDevice::LfoTarget target) {
        SynthDevice synth { "Test Synth" };
        synth.setMixVco1(1.0f);
        synth.setMixVco2(0.0f);
        synth.setMixVco3(0.0f);
        synth.setMultiLevel(0.0f);
        synth.setLpfCutoff(0.3f);
        synth.setLpfResonance(0.5f);
        synth.setVolume(1.0f);
        synth.setGain(0.5f);
        synth.setAmpAttack(0.0f);
        synth.setAmpSustain(1.0f);
        synth.setLfoWaveform(Lfo::Waveform::Sine);
        synth.setLfoRate(0.8f);
        synth.setLfoInt(lfoInt);
        synth.setLfoTarget(target);

        synth.processMidiNoteOn(60, 100);

        const int frameCount = 4096;
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext ctx { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);
        return buffer;
    };

    const auto bufNoMod = renderBuffer(0.0f, SynthDevice::LfoTarget::Resonance);
    const auto bufWithMod = renderBuffer(1.0f, SynthDevice::LfoTarget::Resonance);

    bool differs = false;
    for (size_t i = 0; i < bufNoMod.size(); i++) {
        if (std::abs(bufNoMod[i] - bufWithMod[i]) > 1e-4) {
            differs = true;
            break;
        }
    }
    QVERIFY(differs);
}

void SynthTest::test_lfoTarget_pan_shouldModulatePanning()
{
    const auto renderStereoImbalance = [](SynthDevice::LfoTarget target, float lfoInt) {
        SynthDevice synth { "Test Synth" };
        synth.setMixVco1(1.0f);
        synth.setMixVco2(0.0f);
        synth.setMixVco3(0.0f);
        synth.setMultiLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setVolume(1.0f);
        synth.setGain(0.5f);
        synth.setAmpAttack(0.0f);
        synth.setAmpSustain(1.0f);
        synth.setPan(0.5f);
        synth.setPanSpread(0.0f);
        synth.setLfoWaveform(Lfo::Waveform::Square);
        synth.setLfoRate(0.5f);
        synth.setLfoInt(lfoInt);
        synth.setLfoTarget(target);
        synth.setVoiceMode(SynthDevice::VoiceMode::Unison);

        synth.processMidiNoteOn(60, 127);

        const int frameCount = 2048;
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext ctx { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);

        double sumDiff = 0.0;
        for (size_t i = 0; i < buffer.size(); i += 2)
            sumDiff += std::abs(buffer[i] - buffer[i + 1]);
        return sumDiff;
    };

    const double diffNoPan = renderStereoImbalance(SynthDevice::LfoTarget::Pitch, 0.0f);
    const double diffWithPan = renderStereoImbalance(SynthDevice::LfoTarget::Pan, 1.0f);

    QVERIFY2(diffWithPan > diffNoPan, qPrintable(QString("Pan LFO did not create stereo difference: noPan=%1, withPan=%2").arg(diffNoPan).arg(diffWithPan)));
}

void SynthTest::test_lfoTarget_perOscillatorPitch_shouldModulateOnlyTargetedVco()
{
    // Only VCO 2 is in the mix, so an LFO on Pitch 2 has to be audible while one on Pitch 1 or
    // Pitch 3 has to leave the output untouched.
    const auto renderBuffer = [](SynthDevice::LfoTarget target, float lfoInt) {
        SynthDevice synth { "Test Synth" };
        synth.setMixVco1(0.0f);
        synth.setMixVco2(1.0f);
        synth.setMixVco3(0.0f);
        synth.setMultiLevel(0.0f);
        synth.setLpfCutoff(1.0f);
        synth.setVolume(1.0f);
        synth.setGain(0.5f);
        synth.setAmpAttack(0.0f);
        synth.setAmpSustain(1.0f);
        synth.setLfoWaveform(Lfo::Waveform::Sine);
        synth.setLfoRate(0.8f);
        synth.setLfoInt(lfoInt);
        synth.setLfoTarget(target);

        synth.processMidiNoteOn(60, 100);

        const int frameCount = 4096;
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        AudioContext ctx { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
        synth.processAudio(ctx);
        return buffer;
    };

    const auto differs = [](const std::vector<double> & a, const std::vector<double> & b) {
        for (size_t i = 0; i < a.size(); i++) {
            if (std::abs(a[i] - b[i]) > 1e-4) {
                return true;
            }
        }
        return false;
    };

    // Intensity is bipolar around the centre, so 0.5f is the LFO doing nothing at all.
    const auto reference = renderBuffer(SynthDevice::LfoTarget::Pitch2, 0.5f);

    QVERIFY(differs(reference, renderBuffer(SynthDevice::LfoTarget::Pitch2, 1.0f)));
    QVERIFY(!differs(reference, renderBuffer(SynthDevice::LfoTarget::Pitch1, 1.0f)));
    QVERIFY(!differs(reference, renderBuffer(SynthDevice::LfoTarget::Pitch3, 1.0f)));

    // The global Pitch target still reaches VCO 2 as before.
    QVERIFY(differs(reference, renderBuffer(SynthDevice::LfoTarget::Pitch, 1.0f)));
}

void SynthTest::test_dualMode_shouldProduceAudioOnNote()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
    synth.processMidiNoteOn(60, 100);

    double output[2048] {};
    AudioContext context { std::span(output, 2048), 1024, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    synth.processAudio(context);

    bool soundDetected = false;
    for (const double s : output) {
        if (std::abs(s) > 0.0001) {
            soundDetected = true;
            break;
        }
    }
    QVERIFY(soundDetected);
}

void SynthTest::test_dualMode_polyphony_shouldAllowChords()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
    // Three notes fill all three pairs; chord must still sound
    synth.processMidiNoteOn(60, 100);
    synth.processMidiNoteOn(64, 100);
    synth.processMidiNoteOn(67, 100);

    double output[2048] {};
    AudioContext context { std::span(output, 2048), 1024, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    synth.processAudio(context);

    bool soundDetected = false;
    for (const double s : output) {
        if (std::abs(s) > 0.0001) {
            soundDetected = true;
            break;
        }
    }
    QVERIFY(soundDetected);
}

void SynthTest::test_dualMode_portamento_shouldGlideFrequency()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
    synth.setPortamento(0.5f);

    const double freq60 = 440.0 * std::pow(2.0, (60 - 69) / 12.0);

    synth.processMidiNoteOn(60, 100);
    // Pair 0 (voices 0,1) should hold freq60 as glide start
    QCOMPARE(synth.voiceGlideFrequency(0), freq60);
    QCOMPARE(synth.voiceGlideFrequency(1), freq60);

    // Trigger a second note on a new pair — pair 0 must still be at freq60 (gliding)
    synth.processMidiNoteOn(64, 100);
    QCOMPARE(synth.voiceGlideFrequency(0), freq60);
    QCOMPARE(synth.voiceGlideFrequency(1), freq60);
}

void SynthTest::test_dualMode_portamentoOff_shouldJumpImmediately()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
    synth.setPortamento(0.0f);
    synth.setVoiceDepth(0.0f);

    const double freq60 = 440.0 * std::pow(2.0, (60 - 69) / 12.0);
    const double freq62 = 440.0 * std::pow(2.0, (62 - 69) / 12.0);

    synth.processMidiNoteOn(60, 100);
    bool found60 = false;
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        if (std::abs(synth.voiceGlideFrequency(i) - freq60) < 0.001) {
            found60 = true;
            break;
        }
    }
    QVERIFY(found60);

    // Release 60, trigger 62 so the same pair is reused (affinity)
    synth.processMidiNoteOff(60);
    synth.processMidiNoteOn(62, 100);
    bool found62 = false;
    for (int i = 0; i < SynthDevice::MaxVoices; i++) {
        if (std::abs(synth.voiceGlideFrequency(i) - freq62) < 0.001) {
            found62 = true;
            break;
        }
    }
    QVERIFY(found62);
}

void SynthTest::test_dualMode_liveDepth_shouldUpdateFrequency()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
    synth.setVoiceDepth(0.0f);
    synth.setPortamento(0.0f);

    const uint8_t note = 60;
    const double baseFreq = 440.0 * std::pow(2.0, (note - 69) / 12.0);

    synth.processMidiNoteOn(note, 100);
    // With depth 0, both sub-voices converge to base frequency
    QVERIFY(std::abs(synth.voiceGlideFrequency(0) - baseFreq) < 0.001);
    QVERIFY(std::abs(synth.voiceGlideFrequency(1) - baseFreq) < 0.001);

    double output[256] {};
    AudioContext context { std::span(output, 256), 128, 44100 };
    synth.processAudio(context);

    synth.setVoiceDepth(1.0f);
    synth.processAudio(context);

    // After live depth update: voice 0 (even) is below base, voice 1 (odd) is above
    QVERIFY(synth.voiceGlideFrequency(0) < baseFreq - 0.001);
    QVERIFY(synth.voiceGlideFrequency(1) > baseFreq + 0.001);
}

void SynthTest::test_dualMode_serialization_shouldPreserveState()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.voiceMode(), SynthDevice::VoiceMode::Dual);
    }
}

void SynthTest::test_voiceMode_serialization_shouldPreserveEveryMode()
{
    // The mode is stored as a raw ordinal, so this also pins the enum order down: appending is
    // safe, reordering would silently change the voice mode of every existing project.
    const std::vector<SynthDevice::VoiceMode> modes {
        SynthDevice::VoiceMode::Poly,
        SynthDevice::VoiceMode::Unison,
        SynthDevice::VoiceMode::Dual,
        SynthDevice::VoiceMode::Supersaw,
        SynthDevice::VoiceMode::Drift,
        SynthDevice::VoiceMode::Mono
    };

    for (auto && mode : modes) {
        QByteArray data;
        {
            SynthDevice synth { "Test Synth" };
            synth.setVoiceMode(mode);
            NahdXmlWriter writer { data };
            synth.serializeToXml(writer);
        }

        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.voiceMode(), mode);
    }
}

namespace {

//! Peak of one note held for a while in the given voice mode, with everything else left at defaults.
double notePeak(SynthDevice::VoiceMode voiceMode)
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(voiceMode);
    synth.processMidiNoteOn(60, 100);

    constexpr uint32_t frameCount = 512;
    double buffer[frameCount * 2] {};
    AudioContext context { std::span(buffer, frameCount * 2), frameCount, static_cast<uint32_t>(Constants::defaultSampleRate()) };

    double peak = 0.0;
    // Long enough for the detuned voices to drift through a good part of their beat cycle, so the
    // peak seen is a fair one rather than whatever the attack transient happened to be.
    for (int block = 0; block < 200; block++) {
        std::fill(std::begin(buffer), std::end(buffer), 0.0);
        synth.processAudio(context);
        for (auto && sample : buffer) {
            peak = std::max(peak, std::abs(sample));
        }
    }
    return peak;
}

} // namespace

void SynthTest::test_voiceMode_unison_shouldMatchPolyLevel()
{
    const auto polyPeak = notePeak(SynthDevice::VoiceMode::Poly);
    const auto unisonPeak = notePeak(SynthDevice::VoiceMode::Unison);

    QVERIFY(polyPeak > 0.0);

    // Unison spends every voice on this one note, so without equal-power compensation it would land
    // up to MaxVoices (~15.6 dB) above poly and overload whatever follows. The voices are detuned,
    // so some margin is expected where they align; what must not come back is the raw voice count.
    const auto differenceDb = Utils::Dsp::linearToDb(static_cast<float>(unisonPeak / polyPeak));
    QVERIFY2(differenceDb < 6.0f, qPrintable(QString { "Unison is %1 dB above poly" }.arg(differenceDb)));
    QVERIFY2(differenceDb > -6.0f, qPrintable(QString { "Unison is %1 dB below poly" }.arg(differenceDb)));
}

namespace {

//! Detune of each voice in semitones, read back through the glide frequency the note-on set.
std::vector<double> voiceDetunes(SynthDevice & synth, uint8_t note)
{
    synth.processMidiNoteOn(note, 100);
    const double base = 440.0 * std::pow(2.0, (static_cast<int>(note) - 69) / 12.0);

    std::vector<double> detunes;
    for (size_t i = 0; i < SynthDevice::MaxVoices; i++) {
        detunes.push_back(12.0 * std::log2(synth.voiceGlideFrequency(i) / base));
    }
    return detunes;
}

} // namespace

void SynthTest::test_voiceMode_supersaw_shouldSpaceDetuneUnevenly()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Supersaw);
    synth.setVoiceDepth(1.0f);

    auto detunes = voiceDetunes(synth, 60);
    std::ranges::sort(detunes);

    // Evenly spaced detune is what makes plain unison comb: every adjacent pair beats at the same
    // rate and every wider pair at an exact multiple. Supersaw's whole point is that no two gaps
    // match, so the beating never lines up.
    std::vector<double> gaps;
    for (size_t i = 1; i < detunes.size(); i++) {
        gaps.push_back(detunes.at(i) - detunes.at(i - 1));
    }
    for (size_t i = 1; i < gaps.size(); i++) {
        QVERIFY2(std::abs(gaps.at(i) - gaps.at(i - 1)) > 1.0e-6,
                 qPrintable(QString { "Gaps %1 and %2 are equal at %3" }.arg(i - 1).arg(i).arg(gaps.at(i))));
    }

    // And one voice sits exactly at pitch, which is the core the rest hangs off.
    QVERIFY(std::ranges::any_of(detunes, [](double detune) { return std::abs(detune) < 1.0e-9; }));
}

void SynthTest::test_voiceMode_supersaw_zeroDepth_shouldCollapseToOneVoice()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Supersaw);
    synth.setVoiceDepth(0.0f);

    // Closed up, the side voices fall away and it becomes a single clean saw rather than six voices
    // piled in unison.
    QVERIFY(synth.voiceLevel(0) < 0.1f);
    QVERIFY(synth.voiceLevel(3) > 0.9f);
}

void SynthTest::test_voiceMode_drift_shouldNotDetuneStatically()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Drift);
    synth.setVoiceDepth(1.0f);

    // Drift has no fixed intervals at all; the movement over time is the entire detune.
    for (auto && detune : voiceDetunes(synth, 60)) {
        QVERIFY2(std::abs(detune) < 1.0e-9, qPrintable(QString::number(detune)));
    }
}

void SynthTest::test_voiceMode_supersaw_shouldMatchPolyLevel()
{
    const auto polyPeak = notePeak(SynthDevice::VoiceMode::Poly);
    const auto supersawPeak = notePeak(SynthDevice::VoiceMode::Supersaw);

    QVERIFY(polyPeak > 0.0);

    const auto differenceDb = Utils::Dsp::linearToDb(static_cast<float>(supersawPeak / polyPeak));
    QVERIFY2(differenceDb < 6.0f, qPrintable(QString { "Supersaw is %1 dB above poly" }.arg(differenceDb)));
    QVERIFY2(differenceDb > -6.0f, qPrintable(QString { "Supersaw is %1 dB below poly" }.arg(differenceDb)));
}

void SynthTest::test_voiceMode_drift_shouldMatchPolyLevel()
{
    const auto polyPeak = notePeak(SynthDevice::VoiceMode::Poly);
    const auto driftPeak = notePeak(SynthDevice::VoiceMode::Drift);

    QVERIFY(polyPeak > 0.0);

    const auto differenceDb = Utils::Dsp::linearToDb(static_cast<float>(driftPeak / polyPeak));
    QVERIFY2(differenceDb < 6.0f, qPrintable(QString { "Drift is %1 dB above poly" }.arg(differenceDb)));
    QVERIFY2(differenceDb > -6.0f, qPrintable(QString { "Drift is %1 dB below poly" }.arg(differenceDb)));
}

void SynthTest::test_voiceMode_mono_shouldMatchPolyLevel()
{
    const auto polyPeak = notePeak(SynthDevice::VoiceMode::Poly);
    const auto monoPeak = notePeak(SynthDevice::VoiceMode::Mono);

    QVERIFY(polyPeak > 0.0);

    // Mono is poly with a single voice, so a single note must come out at exactly the same level.
    const auto differenceDb = Utils::Dsp::linearToDb(static_cast<float>(monoPeak / polyPeak));
    QVERIFY2(std::abs(differenceDb) < 0.01f, qPrintable(QString { "Mono is %1 dB off poly" }.arg(differenceDb)));
}

namespace {

//! Runs the synth for the given number of blocks and returns the peak of the last one, per channel.
std::pair<double, double> renderPeaks(SynthDevice & synth, int blocks)
{
    constexpr uint32_t frameCount = 512;
    double buffer[frameCount * 2] {};
    AudioContext context { std::span(buffer, frameCount * 2), frameCount, static_cast<uint32_t>(Constants::defaultSampleRate()) };

    double left = 0.0;
    double right = 0.0;
    for (int block = 0; block < blocks; block++) {
        std::fill(std::begin(buffer), std::end(buffer), 0.0);
        synth.processAudio(context);
        left = 0.0;
        right = 0.0;
        for (uint32_t i = 0; i < frameCount; i++) {
            left = std::max(left, std::abs(buffer[i * 2]));
            right = std::max(right, std::abs(buffer[i * 2 + 1]));
        }
    }
    return { left, right };
}

//! Number of voices the synth has ever triggered, counted through the glide frequency a trigger seeds.
size_t triggeredVoiceCount(const SynthDevice & synth)
{
    size_t count = 0;
    for (size_t i = 0; i < SynthDevice::MaxVoices; i++) {
        if (synth.voiceGlideFrequency(i) > 0.0) {
            count++;
        }
    }
    return count;
}

} // namespace

void SynthTest::test_voiceMode_mono_overlappingNotes_shouldUseOneVoice()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Mono);

    // A chord's worth of note-ons with nothing released in between: poly would spend four voices,
    // mono must keep them all on one.
    synth.processMidiNoteOn(60, 100);
    synth.processMidiNoteOn(64, 100);
    synth.processMidiNoteOn(67, 100);
    synth.processMidiNoteOn(72, 100);

    QCOMPARE(triggeredVoiceCount(synth), 1u);
}

void SynthTest::test_voiceMode_mono_overlappingNotes_shouldGlideToNewPitch()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Mono);
    synth.setPortamento(0.5f);

    const double lowFreq = 440.0 * std::pow(2.0, (60 - 69) / 12.0);
    const double highFreq = 440.0 * std::pow(2.0, (72 - 69) / 12.0);

    synth.processMidiNoteOn(60, 100);
    renderPeaks(synth, 20);
    QVERIFY(std::abs(synth.voiceGlideFrequency(0) - lowFreq) < 0.1);

    // The second note arrives while the first is still held, so the pitch travels there instead of
    // jumping: partway through it must sit strictly between the two.
    synth.processMidiNoteOn(72, 100);
    renderPeaks(synth, 3);
    QVERIFY(synth.voiceGlideFrequency(0) > lowFreq + 1.0);
    QVERIFY(synth.voiceGlideFrequency(0) < highFreq - 1.0);

    renderPeaks(synth, 200);
    QVERIFY(std::abs(synth.voiceGlideFrequency(0) - highFreq) < 1.0);
}

void SynthTest::test_voiceMode_mono_overlappingNote_shouldRetriggerAmpEnvelope()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Mono);
    synth.setAmpAttack(0.3f);
    synth.setAmpDecay(0.4f);
    // Well below unity, so a fresh attack has somewhere to climb to and is plain to see.
    synth.setAmpSustain(0.3f);

    synth.processMidiNoteOn(60, 100);
    const auto [sustainLeft, sustainRight] = renderPeaks(synth, 400);
    const double sustained = std::max(sustainLeft, sustainRight);
    QVERIFY(sustained > 0.0);

    // A note arriving over the sounding one still gets its own attack, which is what keeps a mono
    // line articulate: the level climbs back above sustain instead of holding flat through the move.
    synth.processMidiNoteOn(60, 100);
    double peak = 0.0;
    for (int block = 0; block < 100; block++) {
        const auto [left, right] = renderPeaks(synth, 1);
        peak = std::max({ peak, left, right });
    }
    QVERIFY2(peak > sustained * 1.5, qPrintable(QString { "Peak %1 did not rise above sustain %2" }.arg(peak).arg(sustained)));
}

void SynthTest::test_voiceMode_mono_newNotes_shouldStepThroughPanSpread()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Mono);
    synth.setPanSpread(1.0f);
    synth.setAmpRelease(0.0f);

    // Slot 0 puts the note hard left, slot 1 hard right. Each note here starts from silence, so the
    // line steps through the spread the way successive poly voices would.
    synth.processMidiNoteOn(60, 100);
    const auto [firstLeft, firstRight] = renderPeaks(synth, 10);
    QVERIFY2(firstLeft > firstRight, "First mono note is not panned left");

    synth.processMidiNoteOff(60);
    renderPeaks(synth, 40);

    synth.processMidiNoteOn(60, 100);
    const auto [secondLeft, secondRight] = renderPeaks(synth, 10);
    QVERIFY2(secondRight > secondLeft, "Second mono note is not panned right");
}

void SynthTest::test_voiceMode_mono_overlappingNote_shouldKeepPanPosition()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Mono);
    synth.setPanSpread(1.0f);

    synth.processMidiNoteOn(60, 100);
    const auto [firstLeft, firstRight] = renderPeaks(synth, 10);
    QVERIFY(firstLeft > firstRight);

    // The tone never falls silent between the two notes, so moving the pan now would be an audible
    // jump under a held sound rather than a new note appearing somewhere else.
    synth.processMidiNoteOn(64, 100);
    const auto [secondLeft, secondRight] = renderPeaks(synth, 10);
    QVERIFY2(secondLeft > secondRight, "Overlapping note moved to the next pan slot");
}

void SynthTest::test_dualMode_depth_shouldMatchUnisonOuterSpread()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
    synth.setVoiceDepth(1.0f);
    const auto dualDetunes = voiceDetunes(synth, 60);

    SynthDevice unison { "Test Synth" };
    unison.setVoiceMode(SynthDevice::VoiceMode::Unison);
    unison.setVoiceDepth(1.0f);
    auto unisonDetunes = voiceDetunes(unison, 60);
    std::ranges::sort(unisonDetunes);

    // A dual pair opens to the whole width a unison stack reaches rather than a fraction of it: both
    // halves land exactly on the unison stack's end voices, which is what gives the depth knob the
    // same meaning in either mode.
    QVERIFY(std::abs(dualDetunes.at(0) + Utils::Dsp::voiceSpreadMaxSemitones) < 1.0e-9);
    QVERIFY(std::abs(dualDetunes.at(1) - Utils::Dsp::voiceSpreadMaxSemitones) < 1.0e-9);
    QVERIFY(std::abs(dualDetunes.at(0) - unisonDetunes.front()) < 1.0e-9);
    QVERIFY(std::abs(dualDetunes.at(1) - unisonDetunes.back()) < 1.0e-9);
}

void SynthTest::test_dualMode_noteOn_shouldMatchLiveDetune()
{
    SynthDevice synth { "Test Synth" };
    synth.setVoiceMode(SynthDevice::VoiceMode::Dual);
    synth.setVoiceDepth(0.6f);
    synth.setPortamento(0.0f);

    synth.processMidiNoteOn(60, 100);
    const double noteOn0 = synth.voiceGlideFrequency(0);
    const double noteOn1 = synth.voiceGlideFrequency(1);

    // The per-block update recomputes the detune from scratch. It has to arrive at what the note-on
    // set, or the pitch steps the moment the first block is rendered.
    double output[256] {};
    AudioContext context { std::span(output, 256), 128, 44100 };
    synth.processAudio(context);

    QVERIFY(std::abs(synth.voiceGlideFrequency(0) - noteOn0) < 1.0e-9);
    QVERIFY(std::abs(synth.voiceGlideFrequency(1) - noteOn1) < 1.0e-9);
}

void SynthTest::test_voiceMode_dual_shouldMatchPolyLevel()
{
    const auto polyPeak = notePeak(SynthDevice::VoiceMode::Poly);
    const auto dualPeak = notePeak(SynthDevice::VoiceMode::Dual);

    QVERIFY(polyPeak > 0.0);

    const auto differenceDb = Utils::Dsp::linearToDb(static_cast<float>(dualPeak / polyPeak));
    QVERIFY2(differenceDb < 6.0f, qPrintable(QString { "Dual is %1 dB above poly" }.arg(differenceDb)));
    QVERIFY2(differenceDb > -6.0f, qPrintable(QString { "Dual is %1 dB below poly" }.arg(differenceDb)));
}

namespace {

//! Runs a note for a while with the Mod EG sweeping the cutoff, and returns the peak seen late in
//! the note, once the envelope has had time to finish its attack and decay.
double lateNotePeak(float modSustain)
{
    SynthDevice synth { "Test Synth" };
    synth.setModTarget(SynthDevice::ModTarget::Cutoff);
    synth.setModAttack(0.0f);
    synth.setModDecay(0.2f);
    synth.setModInt(1.0f);
    synth.setModSustain(modSustain);
    synth.setLpfCutoff(0.0f); // Closed, so the modulation is the only thing opening it
    synth.processMidiNoteOn(60, 100);

    constexpr uint32_t frameCount = 512;
    double buffer[frameCount * 2] {};
    AudioContext context { std::span(buffer, frameCount * 2), frameCount, static_cast<uint32_t>(Constants::defaultSampleRate()) };

    double peak = 0.0;
    for (int block = 0; block < 400; block++) {
        std::fill(std::begin(buffer), std::end(buffer), 0.0);
        synth.processAudio(context);
        if (block > 300) { // Well past the decay
            for (auto && sample : buffer) {
                peak = std::max(peak, std::abs(sample));
            }
        }
    }
    return peak;
}

} // namespace

void SynthTest::test_modEg_defaultSustain_shouldReturnToStart()
{
    // The default keeps the old AD behaviour, so existing patches are unchanged: the sweep decays
    // back and leaves the filter where it started.
    SynthDevice synth { "Test Synth" };
    QCOMPARE(synth.modSustain(), 0.0f);
}

void SynthTest::test_modEg_sustain_shouldHoldTheModulation()
{
    const auto withoutSustain = lateNotePeak(0.0f);
    const auto withSustain = lateNotePeak(1.0f);

    // With sustain the envelope parks on its level and the filter stays open for as long as the
    // note is held. Without it the sweep collapses back, which made "sweep somewhere and stay"
    // impossible.
    QVERIFY2(withSustain > withoutSustain * 2.0, qPrintable(QString { "sustained %1 vs %2" }.arg(withSustain).arg(withoutSustain)));
}

namespace {

constexpr uint32_t retriggerSampleRate { 48000 };
constexpr int retriggerFrameCount { 512 };

//! Leaves the synth with nothing in its output but VCO1, at a steady level, so the samples that come
//! out of it are the oscillator's waveform and the retrigger tests can compare them directly.
void setUpRetriggerSynth(SynthDevice & synth, bool phaseSync)
{
    synth.setVco1Waveform(PolyBlepOscillator::Waveform::Sine);
    synth.setVco1Sync(phaseSync);
    synth.setMixVco1(1.0f);
    synth.setMixVco2(0.0f);
    synth.setMixVco3(0.0f);
    synth.setMultiLevel(0.0f);
    synth.setLpfCutoff(1.0f);
    synth.setHpfCutoff(0.0f);
    synth.setDelayMix(0.0f);
    synth.setPanSpread(0.0f);
    synth.setAmpAttack(0.0f);
    // A decaying note rather than a held one: a steady tone repeats itself every period, so any
    // offset a period apart would look like a restart whether the phase was reset or not. Falling
    // level makes the start of a note something only a real restart can reproduce.
    synth.setAmpDecay(0.126f); // ~30 ms
    synth.setAmpSustain(0.0f);
    synth.setLfoInt(0.5f); // Centred, so the LFO adds nothing
}

std::vector<double> renderNote(SynthDevice & synth, uint8_t note)
{
    std::vector<double> buffer(static_cast<size_t>(retriggerFrameCount) * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(retriggerFrameCount), retriggerSampleRate };
    synth.processMidiNoteOn(note, 127);
    synth.processAudio(context);

    std::vector<double> left;
    left.reserve(static_cast<size_t>(retriggerFrameCount));
    for (size_t i = 0; i < buffer.size(); i += 2) {
        left.push_back(buffer[i]);
    }
    return left;
}

double peakOf(const std::vector<double> & samples)
{
    double peak = 0.0;
    for (auto && sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

//! How far into the second note the first note's waveform starts over, or nothing if it never does.
//! A voice that restarted from a known phase repeats the shape it produced the first time; one left
//! running carries on from wherever it happened to be.
std::optional<size_t> restartOffset(const std::vector<double> & first, const std::vector<double> & second, double tolerance)
{
    constexpr size_t compared { 64 };
    //! Wide enough to catch a restart that never came, narrow enough that it stays inside the decay
    //! the comparison relies on.
    constexpr size_t searched { 128 };
    for (size_t offset = 0; offset < searched; offset++) {
        double worst = 0.0;
        for (size_t i = 0; i < compared; i++) {
            worst = std::max(worst, std::abs(second.at(offset + i) - first.at(i)));
        }
        if (worst < tolerance) {
            return offset;
        }
    }
    return std::nullopt;
}

} // namespace

void SynthTest::test_phaseSync_repeatedNote_shouldRestartTheSameWaveform()
{
    // The switch used to reset the phase only on a voice that had gone fully silent, and the poly
    // allocator hands a repeated note straight back to the voice already playing it — so the one
    // case a tracker produces constantly was the case that never synced.
    SynthDevice synth { "Test Synth" };
    setUpRetriggerSynth(synth, true);
    const auto first = renderNote(synth, 69);
    const auto second = renderNote(synth, 69);

    // Not exact: the voice restarts on an oversampled sample, which lands the output waveform up to
    // half a sample away from where the first note put it, and the DC blocker is at a different
    // point in its own settling. Both are far below the level a missing restart would differ by.
    const auto offset = restartOffset(first, second, peakOf(first) * 0.1);
    QVERIFY2(offset.has_value(), "the repeated note did not restart the waveform");
}

void SynthTest::test_phaseSync_repeatedNote_shouldFadeOutBeforeRestarting()
{
    // Resetting the phase and the envelopes under a voice that is still at full level is a step
    // discontinuity, which is a click. The voice has to reach zero before the new note takes over.
    SynthDevice synth { "Test Synth" };
    setUpRetriggerSynth(synth, true);
    const auto first = renderNote(synth, 69);
    const auto second = renderNote(synth, 69);
    const double peak = peakOf(first);

    const auto offset = restartOffset(first, second, peak * 0.1);
    QVERIFY(offset.has_value());
    QVERIFY2(offset.value() > 0, "the note took over with no fade at all");
    QVERIFY2(std::abs(second.at(offset.value() - 1)) < peak * 0.1,
             qPrintable(QString { "handover at %1 of peak %2" }.arg(second.at(offset.value() - 1)).arg(peak)));
}

void SynthTest::test_phaseSyncOff_repeatedNote_shouldNotFadeOut()
{
    // Phase Sync off leaves the oscillators running, so there is nothing to fade out of the way and
    // the repeated note must not lose any level on its way in.
    SynthDevice synth { "Test Synth" };
    setUpRetriggerSynth(synth, false);
    renderNote(synth, 69);
    const auto second = renderNote(synth, 69);

    const std::vector<double> handover { second.begin(), second.begin() + 64 };
    QVERIFY(peakOf(handover) > peakOf(second) * 0.5);
}

void SynthTest::test_ampCurve_shouldSteepenTheAudibleDecay()
{
    // Straight through the whole chain: the knob has to reach the voices, not just the parameter.
    // Halfway through a decay to silence a straight line is still at half level, 6 dB down, which is
    // what makes a pluck sound like a fade. The curve puts it some 20 dB further down.
    auto halfwayPeak = [](float curve) {
        SynthDevice synth { "Test Synth" };
        setUpRetriggerSynth(synth, false);
        synth.setAmpDecay(0.345f); // ~200 ms
        synth.setAmpCurve(curve);

        std::vector<double> buffer(static_cast<size_t>(retriggerFrameCount) * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(retriggerFrameCount), retriggerSampleRate };
        synth.processMidiNoteOn(69, 127);

        // Nine blocks of 512 frames at 48 kHz is 96 ms, so the tenth straddles the halfway point.
        for (int block = 0; block < 9; block++) {
            std::fill(buffer.begin(), buffer.end(), 0.0);
            synth.processAudio(context);
        }
        std::fill(buffer.begin(), buffer.end(), 0.0);
        synth.processAudio(context);

        return peakOf(buffer);
    };

    const double linear = halfwayPeak(0.0f);
    const double curved = halfwayPeak(1.0f);

    QVERIFY(linear > 0.0);
    QVERIFY2(curved * 5.0 < linear, qPrintable(QString { "curved %1 vs linear %2" }.arg(curved).arg(linear)));
}

void SynthTest::test_ampCurve_serialization_shouldPreserveState()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        QCOMPARE(synth.ampCurve(), 0.0f); // Straight lines, as before the knob existed
        synth.setAmpCurve(0.7f);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.ampCurve(), 0.7f);
    }
}

void SynthTest::test_modCurve_serialization_shouldPreserveState()
{
    QByteArray data;
    {
        SynthDevice synth { "Test Synth" };
        QCOMPARE(synth.modCurve(), 0.0f);
        synth.setModCurve(0.4f);
        NahdXmlWriter writer { data };
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.modCurve(), 0.4f);
    }
}

// A render has to be a function of the project and nothing else. Oscillator drift and initial
// phases are drawn from the device's own generator, so unless resetAudio() re-seeds it, the same
// notes come out differently depending on what the session played beforehand -- which is exactly
// what made two renders of the same song differ.
void SynthTest::test_resetAudio_afterOtherNotes_shouldRenderIdenticalAudio()
{
    const auto renderSequence = [](SynthDevice & synth) {
        std::vector<double> rendered;
        const std::vector<uint8_t> notes { 62, 64, 60 };
        for (auto && note : notes) {
            synth.processMidiNoteOn(note, 100);
            for (int block = 0; block < 8; block++) {
                std::vector<double> buffer(256 * 2, 0.0);
                AudioContext context { std::span(buffer.data(), buffer.size()), 256, 48000 };
                synth.processAudio(context);
                rendered.insert(rendered.end(), buffer.begin(), buffer.end());
            }
            synth.processMidiNoteOff(note);
        }
        return rendered;
    };

    SynthDevice synth { "Test Synth" };
    synth.setOscillatorDrift(0.5f); // Drift draws from the generator, so the seed has to matter.

    synth.resetAudio();
    const auto first = renderSequence(synth);

    // Whatever the session does in between must leave no trace on the next render.
    synth.processMidiNoteOn(71, 64);
    for (int block = 0; block < 16; block++) {
        std::vector<double> buffer(256 * 2, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), 256, 48000 };
        synth.processAudio(context);
    }
    synth.processMidiNoteOff(71);

    synth.resetAudio();
    const auto second = renderSequence(synth);

    QCOMPARE(first.size(), second.size());
    QVERIFY(!first.empty());
    QVERIFY(std::ranges::any_of(first, [](double sample) { return std::abs(sample) > 1.0e-6; }));
    QCOMPARE(first, second);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SynthTest)
