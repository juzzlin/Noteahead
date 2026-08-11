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

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>
#include <algorithm>
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
    QCOMPARE(synth.volume(), Device::faderPositionFromMidiCc(64));

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
    NahdXmlWriter writer { xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    NahdXmlReader reader { xml };
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
    NahdXmlWriter writer { xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    NahdXmlReader reader { xml };
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
    NahdXmlWriter writer { xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    NahdXmlReader reader { xml };
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
    NahdXmlWriter writer { xml };
    synth1.serializeToXml(writer);

    WavetableSynthDevice synth2 { "Test Synth 2" };
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        synth2.deserializeFromXml(reader);
    }

    QCOMPARE(synth2.lfo2Waveform(), Lfo::Waveform::Random);
}

void WavetableSynthTest::test_midiCcModWheel_shouldOverrideLfoIntensity()
{
    WavetableSynthDevice synth { "Test Synth" };
    synth.setLfoInt(0.5f); // zero intensity (midpoint)

    synth.processMidiCc(1, 127, 0);
    QCOMPARE(synth.lfoInt(), 1.0f);

    synth.processMidiCc(1, 0, 0);
    QCOMPARE(synth.lfoInt(), 0.0f);
}

static void setupBasicSynth(WavetableSynthDevice & synth)
{
    synth.setOsc1Level(1.0f);
    synth.setOsc2Level(0.0f);
    synth.setNoiseLevel(0.0f);
    synth.setLpfCutoff(0.4f);
    synth.setLpfResonance(0.5f);
    synth.setVolume(1.0f);
    synth.setAmpAttack(0.0f);
    synth.setAmpSustain(1.0f);
    synth.setLfoWaveform(Lfo::Waveform::Sine);
    synth.setLfoRate(0.8f);
}

static std::vector<double> renderBuffer(WavetableSynthDevice & synth)
{
    synth.processMidiNoteOn(60, 100);
    const int frameCount = 4096;
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext ctx { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frameCount), 44100 };
    synth.processAudio(ctx);
    return buffer;
}

void WavetableSynthTest::test_lfoTarget_volume_shouldModulateAmplitude()
{
    WavetableSynthDevice synthNoMod { "Test Synth" };
    setupBasicSynth(synthNoMod);
    synthNoMod.setLfoInt(0.5f); // zero intensity
    synthNoMod.setLfoTarget(WavetableSynthDevice::LfoTarget::Volume);
    const auto bufNoMod = renderBuffer(synthNoMod);

    WavetableSynthDevice synthWithMod { "Test Synth" };
    setupBasicSynth(synthWithMod);
    synthWithMod.setLfoInt(1.0f);
    synthWithMod.setLfoTarget(WavetableSynthDevice::LfoTarget::Volume);
    const auto bufWithMod = renderBuffer(synthWithMod);

    bool differs = false;
    for (size_t i = 0; i < bufNoMod.size(); i++) {
        if (std::abs(bufNoMod[i] - bufWithMod[i]) > 1e-4) {
            differs = true;
            break;
        }
    }
    QVERIFY(differs);
}

void WavetableSynthTest::test_lfoTarget_resonance_shouldModulateResonance()
{
    WavetableSynthDevice synthNoMod { "Test Synth" };
    setupBasicSynth(synthNoMod);
    synthNoMod.setLfoInt(0.5f);
    synthNoMod.setLfoTarget(WavetableSynthDevice::LfoTarget::Resonance);
    const auto bufNoMod = renderBuffer(synthNoMod);

    WavetableSynthDevice synthWithMod { "Test Synth" };
    setupBasicSynth(synthWithMod);
    synthWithMod.setLfoInt(1.0f);
    synthWithMod.setLfoTarget(WavetableSynthDevice::LfoTarget::Resonance);
    const auto bufWithMod = renderBuffer(synthWithMod);

    bool differs = false;
    for (size_t i = 0; i < bufNoMod.size(); i++) {
        if (std::abs(bufNoMod[i] - bufWithMod[i]) > 1e-4) {
            differs = true;
            break;
        }
    }
    QVERIFY(differs);
}

void WavetableSynthTest::test_lfoTarget_pan_shouldModulatePanning()
{
    WavetableSynthDevice synth { "Test Synth" };
    setupBasicSynth(synth);
    synth.setLfoInt(1.0f);
    synth.setLfoTarget(WavetableSynthDevice::LfoTarget::Pan);
    synth.setPan(0.5f);
    synth.setPanSpread(0.0f);
    synth.setVoiceMode(WavetableSynthDevice::VoiceMode::Unison);

    const auto buf = renderBuffer(synth);

    double sumDiff = 0.0;
    for (size_t i = 0; i < buf.size(); i += 2)
        sumDiff += std::abs(buf[i] - buf[i + 1]);

    QVERIFY2(sumDiff > 0.001, "Pan LFO did not create any stereo difference");
}

void WavetableSynthTest::test_lfo2Target_volume_shouldModulateAmplitude()
{
    WavetableSynthDevice synthNoMod { "Test Synth" };
    setupBasicSynth(synthNoMod);
    synthNoMod.setLfo2Int(0.5f);
    synthNoMod.setLfo2Target(WavetableSynthDevice::LfoTarget::Volume);
    const auto bufNoMod = renderBuffer(synthNoMod);

    WavetableSynthDevice synthWithMod { "Test Synth" };
    setupBasicSynth(synthWithMod);
    synthWithMod.setLfo2Int(1.0f);
    synthWithMod.setLfo2Target(WavetableSynthDevice::LfoTarget::Volume);
    const auto bufWithMod = renderBuffer(synthWithMod);

    bool differs = false;
    for (size_t i = 0; i < bufNoMod.size(); i++) {
        if (std::abs(bufNoMod[i] - bufWithMod[i]) > 1e-4) {
            differs = true;
            break;
        }
    }
    QVERIFY(differs);
}

void WavetableSynthTest::test_lfo2Target_resonance_shouldModulateResonance()
{
    WavetableSynthDevice synthNoMod { "Test Synth" };
    setupBasicSynth(synthNoMod);
    synthNoMod.setLfo2Int(0.5f);
    synthNoMod.setLfo2Target(WavetableSynthDevice::LfoTarget::Resonance);
    const auto bufNoMod = renderBuffer(synthNoMod);

    WavetableSynthDevice synthWithMod { "Test Synth" };
    setupBasicSynth(synthWithMod);
    synthWithMod.setLfo2Int(1.0f);
    synthWithMod.setLfo2Target(WavetableSynthDevice::LfoTarget::Resonance);
    const auto bufWithMod = renderBuffer(synthWithMod);

    bool differs = false;
    for (size_t i = 0; i < bufNoMod.size(); i++) {
        if (std::abs(bufNoMod[i] - bufWithMod[i]) > 1e-4) {
            differs = true;
            break;
        }
    }
    QVERIFY(differs);
}

void WavetableSynthTest::test_lfo2Target_pan_shouldModulatePanning()
{
    WavetableSynthDevice synth { "Test Synth" };
    setupBasicSynth(synth);
    synth.setLfo2Int(1.0f);
    synth.setLfo2Target(WavetableSynthDevice::LfoTarget::Pan);
    synth.setPan(0.5f);
    synth.setPanSpread(0.0f);
    synth.setVoiceMode(WavetableSynthDevice::VoiceMode::Unison);

    const auto buf = renderBuffer(synth);

    double sumDiff = 0.0;
    for (size_t i = 0; i < buf.size(); i += 2)
        sumDiff += std::abs(buf[i] - buf[i + 1]);

    QVERIFY2(sumDiff > 0.001, "LFO2 Pan did not create any stereo difference");
}

namespace {

//! Peak of one note held for a while in the given voice mode, with everything else left at defaults.
double notePeak(WavetableSynthDevice::VoiceMode voiceMode)
{
    WavetableSynthDevice synth { "Test Synth" };
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

void WavetableSynthTest::test_voiceMode_unison_shouldMatchPolyLevel()
{
    const auto polyPeak = notePeak(WavetableSynthDevice::VoiceMode::Poly);
    const auto unisonPeak = notePeak(WavetableSynthDevice::VoiceMode::Unison);

    QVERIFY(polyPeak > 0.0);

    // Unison spends every voice on this one note, so without equal-power compensation it would land
    // up to MaxVoices (~18 dB) above poly and overload whatever follows. The voices are detuned, so
    // some margin is expected where they align; what must not come back is the raw voice count.
    const auto differenceDb = Utils::Dsp::linearToDb(static_cast<float>(unisonPeak / polyPeak));
    QVERIFY2(differenceDb < 6.0f, qPrintable(QString { "Unison is %1 dB above poly" }.arg(differenceDb)));
    QVERIFY2(differenceDb > -6.0f, qPrintable(QString { "Unison is %1 dB below poly" }.arg(differenceDb)));
}

void WavetableSynthTest::test_lfoIntensity_shouldApplyTheDepthItReadsOut()
{
    // The Intensity knob reads out through a cubic taper, so the engine has to modulate by the
    // amount shown rather than by the raw knob position. Aimed at Volume, the depth can be read
    // straight back off the audio: the envelope runs between 1 + depth and 1 - depth.
    const auto measuredDepth = [](float knobPosition) {
        WavetableSynthDevice synth { "Test Synth" };
        setupBasicSynth(synth);
        synth.setLpfCutoff(1.0f);
        synth.setLpfResonance(0.0f);
        synth.setLfoWaveform(Lfo::Waveform::Sine);
        synth.setLfoRate(0.9f);
        synth.setLfoTarget(WavetableSynthDevice::LfoTarget::Volume);
        synth.setLfoInt(0.5f + knobPosition * 0.5f);
        const auto buffer = renderBuffer(synth);

        // The carrier is far faster than the LFO, so the loudest sample in a short window tracks
        // the envelope.
        constexpr size_t Window = 256;
        double loudest = 0.0;
        double quietest = 1.0;
        for (size_t start = 0; start + Window * 2 <= buffer.size(); start += Window * 2) {
            double windowPeak = 0.0;
            for (size_t i = start; i < start + Window * 2; i += 2) {
                windowPeak = std::max(windowPeak, std::abs(buffer[i]));
            }
            loudest = std::max(loudest, windowPeak);
            quietest = std::min(quietest, windowPeak);
        }

        return (loudest + quietest) > 0.0 ? (loudest - quietest) / (loudest + quietest) : 0.0;
    };

    for (const double knobPosition : { 0.3, 0.6, 0.8, 1.0 }) {
        const auto expected = knobPosition * knobPosition * knobPosition;
        const auto actual = measuredDepth(static_cast<float>(knobPosition));
        QVERIFY2(std::abs(actual - expected) < 0.05,
                 qPrintable(QString { "Knob at %1 reads out %2 but modulates by %3" }.arg(knobPosition).arg(expected).arg(actual)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WavetableSynthTest)
