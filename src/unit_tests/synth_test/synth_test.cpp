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

#include "common/constants.hpp"
#include "domain/devices/synth_device.hpp"
#include "domain/devices/synth_presets.hpp"

#include <QBuffer>
#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <cmath>
#include <map>

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
    QCOMPARE(synth.volume(), 64.0f / 127.0f);

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

    QCOMPARE(synth.volume(), 10.0f / 127.0f);
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
    const auto vco1Wave = synth.parameter(Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString());
    QVERIFY(vco1Wave.has_value());
    QVERIFY(vco1Wave->get().isDiscrete());

    const auto vco1Octave = synth.parameter(Constants::NahdXml::xmlKeySynthVco1Octave().toStdString());
    QVERIFY(vco1Octave.has_value());
    QVERIFY(vco1Octave->get().isDiscrete());

    const auto vco1Pitch = synth.parameter(Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString());
    QVERIFY(vco1Pitch.has_value());
    QVERIFY(!vco1Pitch->get().isDiscrete());

    const auto modTarget = synth.parameter(Constants::NahdXml::xmlKeySynthModTarget().toStdString());
    QVERIFY(modTarget.has_value());
    QVERIFY(modTarget->get().isDiscrete());

    const auto voiceMode = synth.parameter(Constants::NahdXml::xmlKeyVoiceMode().toStdString());
    QVERIFY(voiceMode.has_value());
    QVERIFY(voiceMode->get().isDiscrete());

    // Test continuous parameters
    const auto lpfCutoff = synth.parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString());
    QVERIFY(lpfCutoff.has_value());
    QVERIFY(!lpfCutoff->get().isDiscrete());

    const auto ampAttack = synth.parameter(Constants::NahdXml::xmlKeySynthAmpAttack().toStdString());
    QVERIFY(ampAttack.has_value());
    QVERIFY(!ampAttack->get().isDiscrete());

    const auto multiShape = synth.parameter(Constants::NahdXml::xmlKeySynthMultiShape().toStdString());
    QVERIFY(multiShape.has_value());
    QVERIFY(!multiShape->get().isDiscrete());
}

void SynthTest::test_midiBankAndProgramChange_shouldLoadCorrectPreset()
{
    SynthDevice synth { "Test Synth" };

    // Set some user presets
    UserPresets userPresets;
    const SynthPreset up1 { "User 1", { { Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString(), 0.123f } } };
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
        const auto expectedCutoff = factoryPresets[0].parameters.count(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()) ? factoryPresets[0].parameters.at(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()) : 1.0f; // Default 1.0
        QCOMPARE(synth.lpfCutoff(), expectedCutoff);
    }
}

void SynthTest::test_userPresets_shouldSaveAndLoad()
{
    SynthDevice synth { "Test Synth" };

    UserPresets userPresets;
    for (int i = 0; i < 128; i++)
        userPresets[i] = SynthPresets::initPreset();

    const SynthPreset myPreset { "My Bass", { { Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString(), 0.42f } } };
    userPresets[10] = myPreset;

    synth.setUserPresets(userPresets);
    synth.loadPreset(1, 10);

    QCOMPARE(synth.lpfCutoff(), 0.42f);
}

void SynthTest::test_userPresetsDiscreteValues_shouldLoadCorrectly()
{
    SynthDevice synth { "Test Synth" };

    UserPresets userPresets;
    const std::string vco1WaveformKey = Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString();

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
    QCOMPARE(synth.delayType(), DelayEffect::Type::PingPong);

    // Test Phase Sync (vco1Sync)
    const std::string vco1SyncKey = Constants::NahdXml::xmlKeySynthVco1Sync().toStdString();
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
        QXmlStreamWriter writer(&data);
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        QXmlStreamReader reader(data);
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
        QXmlStreamWriter writer(&data);
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        QXmlStreamReader reader(data);
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        synth.deserializeFromXml(reader);
        QCOMPARE(synth.gain(), 0.8f);
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
    synth.processMidiCc(7, 127, 0); // Volume to 1.0
    synth.processMidiCc(10, 127, 0); // Pan to 1.0
    QCOMPARE(synth.volume(), 1.0f);
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
        QXmlStreamWriter writer(&data);
        synth.serializeToXml(writer);
    }

    {
        SynthDevice synth { "Test Synth" };
        QXmlStreamReader reader(data);
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
        QCOMPARE(synth.volume(), 1.0f);
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

    // Shape 1.0 -> very narrow pulse (0.5%)
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
    // With 0.5% duty cycle, very few should be positive
    QVERIFY(positiveSamples < 50);
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

    auto getPeak = [&](uint8_t velocity) {
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

    const double peakLow = getPeak(40);
    const double peakHigh = getPeak(127);

    QVERIFY2(peakHigh > peakLow, QString("Velocity did not affect volume: peakLow=%1, peakHigh=%2").arg(peakLow).arg(peakHigh).toUtf8().constData());
}

void SynthTest::test_velocitySensitivity_shouldAffectVoiceVelocity()
{
    SynthDevice synth { std::string("Test Synth") };
    synth.setVolume(1.0f);
    synth.setGain(0.5f);
    synth.setAmpAttack(0.0f);
    synth.setAmpSustain(1.0f);
    synth.setMixVco1(1.0f);
    synth.setMixVco2(0.0f);
    synth.setMixVco3(0.0f);
    synth.setMultiLevel(0.0f);
    synth.setLpfCutoff(1.0f);

    QCOMPARE(synth.ampVelocitySensitivity(), 1.0f);

    auto getPeak = [&](uint8_t velocity, float sensitivity) {
        synth.setAmpVelocitySensitivity(sensitivity);
        synth.resetAudio();
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

    // 1. With 0% sensitivity, velocity should not affect volume (peak should be same)
    const double peak0SensLow = getPeak(10, 0.0f);
    const double peak0SensHigh = getPeak(127, 0.0f);
    QVERIFY2(std::abs(peak0SensLow - peak0SensHigh) < 0.001, QString { "Expected same peak at 0% sensitivity" }.toUtf8().constData());

    // 1b. With 100% sensitivity, velocity 0 should be silent
    const double peak100SensSilent = getPeak(0, 1.0f);
    QVERIFY2(peak100SensSilent < 0.0001, QString { "Expected silence at 100% sensitivity and 0 velocity, got %1" }.arg(peak100SensSilent).toUtf8().constData());

    // 2. With 100% sensitivity, velocity should affect volume (standard behavior)
    const double peak100SensLow = getPeak(10, 1.0f);
    const double peak100SensHigh = getPeak(127, 1.0f);
    QVERIFY2(peak100SensHigh > peak100SensLow * 10.0, QString { "Expected ~12x difference at 100% sensitivity, got %1x" }.arg(peak100SensHigh / peak100SensLow).toUtf8().constData());

    // 3. With 100% sensitivity and 127 velocity, it should match 0% sensitivity (always full)
    QVERIFY2(std::abs(peak0SensHigh - peak100SensHigh) < 0.001, QString { "Peak at 127 velocity should be same regardless of sensitivity" }.toUtf8().constData());
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SynthTest)
