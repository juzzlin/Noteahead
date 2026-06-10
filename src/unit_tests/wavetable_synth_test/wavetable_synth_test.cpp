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

#include "wavetable_synth_test.hpp"

#include "common/constants.hpp"
#include "domain/devices/wavetable_synth_device.hpp"

#include <QTest>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <cmath>

namespace noteahead {

void WavetableSynthTest::test_name_shouldReturnCorrectName()
{
    const std::string name = "Test Synth";
    const WavetableSynthDevice synth { name };
    QCOMPARE(synth.name(), name);
}

void WavetableSynthTest::test_defaultValues_shouldBeCorrect()
{
    const WavetableSynthDevice synth { "Test Synth" };
    QCOMPARE(synth.osc1Pos(), 0.0f);
    QCOMPARE(synth.osc1Octave(), 0);
    QCOMPARE(synth.osc1Pitch(), 0.5f);
    QCOMPARE(synth.osc1Level(), 1.0f);
    QCOMPARE(synth.osc2Pos(), 0.5f);
    QCOMPARE(synth.osc2Octave(), 0);
    QCOMPARE(synth.osc2Pitch(), 0.5f);
    QCOMPARE(synth.osc2Level(), 0.0f);
    QCOMPARE(synth.noiseLevel(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 1.0f);
    QCOMPARE(synth.lpfResonance(), 0.0f);
    QCOMPARE(synth.ampSustain(), 1.0f);
    QCOMPARE(synth.voiceMode(), WavetableSynthDevice::VoiceMode::Poly);
    QCOMPARE(synth.pitchBendRange(), 2);
}

void WavetableSynthTest::test_parameterSetting_shouldUpdateValues()
{
    WavetableSynthDevice synth { "Test Synth" };

    synth.setOsc1Pos(0.75f);
    QCOMPARE(synth.osc1Pos(), 0.75f);

    synth.setOsc1Octave(1);
    QCOMPARE(synth.osc1Octave(), 1);

    synth.setLpfCutoff(0.5f);
    QCOMPARE(synth.lpfCutoff(), 0.5f);

    synth.setLfoRate(0.8f);
    QCOMPARE(synth.lfoRate(), 0.8f);

    synth.setModInt(0.4f);
    QCOMPARE(synth.modInt(), 0.4f);

    synth.setVoiceMode(WavetableSynthDevice::VoiceMode::Unison);
    QCOMPARE(synth.voiceMode(), WavetableSynthDevice::VoiceMode::Unison);
}

void WavetableSynthTest::test_polyphony_shouldActiveMultipleVoices()
{
    WavetableSynthDevice synth { "Test Synth" };
    synth.processMidiNoteOn(60, 100);
    synth.processMidiNoteOn(64, 100);
    synth.processMidiNoteOn(67, 100);

    const uint32_t frameCount = 128;
    std::vector<double> buffer(frameCount * 2, 0.0);
    AudioContext context { std::span<double> { buffer.data(), buffer.size() }, frameCount, 44100 };

    synth.processAudio(context);

    bool hasAudio = false;
    for (const double sample : buffer) {
        if (std::abs(sample) > 0.0001) {
            hasAudio = true;
            break;
        }
    }
    QVERIFY(hasAudio);
}

void WavetableSynthTest::test_midiCc_shouldUpdateParameters()
{
    WavetableSynthDevice synth { "Test Synth" };

    // CC 7 (Volume)
    synth.processMidiCc(7, 64, 0);
    QCOMPARE(synth.volume(), 64.0f / 127.0f);

    // CC 10 (Pan)
    synth.processMidiCc(10, 32, 0);
    QCOMPARE(synth.pan(), 32.0f / 127.0f);

    // CC 74 (Cutoff)
    synth.processMidiCc(74, 100, 0);
    QCOMPARE(synth.lpfCutoff(), 100.0f / 127.0f);

    // CC 71 (Resonance)
    synth.processMidiCc(71, 50, 0);
    QCOMPARE(synth.lpfResonance(), 50.0f / 127.0f);
}

void WavetableSynthTest::test_reset_shouldRestoreDefaults()
{
    WavetableSynthDevice synth { "Test Synth" };
    synth.setOsc1Pos(0.8f);
    synth.setLpfCutoff(0.3f);
    synth.processMidiNoteOn(60, 100);

    synth.reset();

    QCOMPARE(synth.osc1Pos(), 0.0f);
    QCOMPARE(synth.lpfCutoff(), 1.0f);
    QVERIFY(!synth.hasActiveAudio());
}

void WavetableSynthTest::test_serialization_shouldPreserveState()
{
    WavetableSynthDevice synth1 { "Test Synth 1" };
    const float pos = 0.8f;
    const float cutoff = 0.3f;
    const int wtIndex = 1;
    synth1.setOsc1Pos(pos);
    synth1.setLpfCutoff(cutoff);
    synth1.setWavetableIndex(wtIndex);

    QString xml;
    QXmlStreamWriter writer { &xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    QXmlStreamReader reader { xml };
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.osc1Pos(), pos);
    QCOMPARE(synth2.lpfCutoff(), cutoff);
    QCOMPARE(synth2.wavetableIndex(), wtIndex);
}

void WavetableSynthTest::test_pitchBend_shouldUpdateFrequency()
{
    WavetableSynthDevice synth { "Test Synth" };
    synth.processMidiNoteOn(60, 100);

    const uint32_t frameCount = 128;
    std::vector<double> buffer1(frameCount * 2, 0.0);
    AudioContext context1 { std::span<double> { buffer1.data(), buffer1.size() }, frameCount, 44100 };
    synth.processAudio(context1);

    synth.resetAudio();
    synth.processMidiNoteOn(60, 100);
    synth.processMidiPitchBend(16383, 0); // Max pitch bend up

    std::vector<double> buffer2(frameCount * 2, 0.0);
    AudioContext context2 { std::span<double> { buffer2.data(), buffer2.size() }, frameCount, 44100 };
    synth.processAudio(context2);

    // Buffers should be different due to pitch bend
    bool different = false;
    for (size_t i = 0; i < buffer1.size(); i++) {
        if (std::abs(buffer1[i] - buffer2[i]) > 0.0001) {
            different = true;
            break;
        }
    }
    QVERIFY(different);
}

void WavetableSynthTest::test_audio_shouldProcessWhenActive()
{
    WavetableSynthDevice synth { "Test Synth" };
    synth.processMidiNoteOn(60, 100);

    const uint32_t frameCount = 128;
    std::vector<double> buffer(frameCount * 2, 0.0);
    AudioContext context { std::span<double> { buffer.data(), buffer.size() }, frameCount, 44100 };

    synth.processAudio(context);

    bool hasAudio = false;
    for (const double sample : buffer) {
        if (std::abs(sample) > 0.0001) {
            hasAudio = true;
            break;
        }
    }
    QVERIFY(hasAudio);
}

void WavetableSynthTest::test_hpf_shouldUpdateParameterAndFilterAudio()
{
    WavetableSynthDevice synth { "Test Synth" };

    // Test parameter setting
    synth.setHpfCutoff(0.5f);
    QCOMPARE(synth.hpfCutoff(), 0.5f);

    // Test MIDI CC
    synth.processMidiCc(81, 64, 0);
    QCOMPARE(synth.hpfCutoff(), 64.0f / 127.0f);

    // Test audio processing with HPF
    synth.setHpfCutoff(0.0f);
    synth.processMidiNoteOn(60, 100);

    const uint32_t frameCount = 512;
    std::vector<double> buffer1(frameCount * 2, 0.0);
    AudioContext context1 { std::span<double> { buffer1.data(), buffer1.size() }, frameCount, 44100 };
    synth.processAudio(context1);

    synth.resetAudio();
    synth.processMidiNoteOn(60, 100);
    synth.setHpfCutoff(0.9f); // High cutoff should significantly change the audio

    std::vector<double> buffer2(frameCount * 2, 0.0);
    AudioContext context2 { std::span<double> { buffer2.data(), buffer2.size() }, frameCount, 44100 };
    synth.processAudio(context2);

    bool different = false;
    for (size_t i = 0; i < buffer1.size(); i++) {
        if (std::abs(buffer1[i] - buffer2[i]) > 0.0001) {
            different = true;
            break;
        }
    }
    QVERIFY(different);
}

void WavetableSynthTest::test_wavetableSelection_shouldUpdateWavetable()
{
    WavetableSynthDevice synth { "Test Synth" };

    const auto names = synth.wavetableNames();
    QVERIFY(names.size() >= 2);
    QCOMPARE(synth.wavetableIndex(), 0);

    synth.setWavetableIndex(1);
    QCOMPARE(synth.wavetableIndex(), 1);

    // Test audio processing with different wavetables
    synth.setWavetableIndex(0);
    synth.processMidiNoteOn(60, 100);

    const uint32_t frameCount = 128;
    std::vector<double> buffer1(frameCount * 2, 0.0);
    AudioContext context1 { std::span<double> { buffer1.data(), buffer1.size() }, frameCount, 44100 };
    synth.processAudio(context1);

    synth.resetAudio();
    synth.setWavetableIndex(1);
    synth.processMidiNoteOn(60, 100);

    std::vector<double> buffer2(frameCount * 2, 0.0);
    AudioContext context2 { std::span<double> { buffer2.data(), buffer2.size() }, frameCount, 44100 };
    synth.processAudio(context2);

    bool different = false;
    for (size_t i = 0; i < buffer1.size(); i++) {
        if (std::abs(buffer1[i] - buffer2[i]) > 0.0001) {
            different = true;
            break;
        }
    }
    QVERIFY(different);
}

void WavetableSynthTest::test_lfo2_defaultValues_shouldBeCorrect()
{
    const WavetableSynthDevice synth { "Test Synth" };
    QCOMPARE(synth.lfo2Waveform(), Lfo::Waveform::Triangle);
    QCOMPARE(synth.lfo2Mode(), Lfo::Mode::Normal);
    QCOMPARE(synth.lfo2Rate(), 0.5f);
    QCOMPARE(synth.lfo2Int(), 0.5f);
    QCOMPARE(synth.lfo2Target(), WavetableSynthDevice::LfoTarget::Pitch);
}

void WavetableSynthTest::test_lfo2_parameterSetting_shouldUpdateValues()
{
    WavetableSynthDevice synth { "Test Synth" };

    synth.setLfo2Waveform(Lfo::Waveform::Sine);
    QCOMPARE(synth.lfo2Waveform(), Lfo::Waveform::Sine);

    synth.setLfo2Mode(Lfo::Mode::BPM);
    QCOMPARE(synth.lfo2Mode(), Lfo::Mode::BPM);

    synth.setLfo2Rate(0.75f);
    QCOMPARE(synth.lfo2Rate(), 0.75f);

    synth.setLfo2Int(0.3f);
    QCOMPARE(synth.lfo2Int(), 0.3f);

    synth.setLfo2Target(WavetableSynthDevice::LfoTarget::Cutoff);
    QCOMPARE(synth.lfo2Target(), WavetableSynthDevice::LfoTarget::Cutoff);
}

void WavetableSynthTest::test_lfo2_serialization_shouldPreserveState()
{
    WavetableSynthDevice synth1 { "Test Synth 1" };
    synth1.setLfo2Waveform(Lfo::Waveform::Sine);
    synth1.setLfo2Mode(Lfo::Mode::BPM);
    synth1.setLfo2Rate(0.75f);
    synth1.setLfo2Int(0.3f);
    synth1.setLfo2Target(WavetableSynthDevice::LfoTarget::Cutoff);

    QString xml;
    QXmlStreamWriter writer { &xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    QXmlStreamReader reader { xml };
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.lfo2Waveform(), Lfo::Waveform::Sine);
    QCOMPARE(synth2.lfo2Mode(), Lfo::Mode::BPM);
    QCOMPARE(synth2.lfo2Rate(), 0.75f);
    QCOMPARE(synth2.lfo2Int(), 0.3f);
    QCOMPARE(synth2.lfo2Target(), WavetableSynthDevice::LfoTarget::Cutoff);
}

void WavetableSynthTest::test_lfoWaveform_random_serialization_shouldPreserveState()
{
    WavetableSynthDevice synth1 { "Test Synth 1" };
    synth1.setLfoWaveform(Lfo::Waveform::Random);

    QString xml;
    QXmlStreamWriter writer { &xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    QXmlStreamReader reader { xml };
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.lfoWaveform(), Lfo::Waveform::Random);
}

void WavetableSynthTest::test_lfo2Waveform_random_serialization_shouldPreserveState()
{
    WavetableSynthDevice synth1 { "Test Synth 1" };
    synth1.setLfo2Waveform(Lfo::Waveform::Random);

    QString xml;
    QXmlStreamWriter writer { &xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    QXmlStreamReader reader { xml };
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.lfo2Waveform(), Lfo::Waveform::Random);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WavetableSynthTest)
