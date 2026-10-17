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

#include "ju_synth_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/ju_synth_device.hpp"
#include "../../domain/devices/ju_synth_presets.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

#include <cmath>
#include <set>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr double MiddleC { 261.6255653 };

//! Plays a note, lets it settle, and returns the next 8192 frames, interleaved.
std::vector<double> render(JuSynthDevice & synth, uint8_t note = 60, size_t frames = 8192)
{
    synth.processMidiNoteOn(note, 127);
    std::vector<double> warmUp(2048 * 2, 0.0);
    AudioContext warmUpContext { std::span(warmUp.data(), warmUp.size()), 2048, SampleRate };
    synth.processAudio(warmUpContext);

    std::vector<double> buffer(frames * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), static_cast<uint32_t>(frames), SampleRate };
    synth.processAudio(context);
    return buffer;
}

//! Magnitude at one frequency in the left channel of an interleaved buffer, by correlation under a
//! Hann window, so that a strong note nearby does not leak into the one being measured.
double magnitudeAt(const std::vector<double> & buffer, double hz)
{
    const size_t frames = buffer.size() / 2;
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < frames; i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(frames));
        const double phase = 2.0 * M_PI * hz * static_cast<double>(i) / SampleRate;
        re += window * buffer[i * 2] * std::cos(phase);
        im += window * buffer[i * 2] * std::sin(phase);
    }
    return std::hypot(re, im) / static_cast<double>(frames);
}

double rms(const std::vector<double> & buffer)
{
    double sum = 0.0;
    for (auto && sample : buffer) {
        sum += sample * sample;
    }
    return std::sqrt(sum / static_cast<double>(buffer.size()));
}

} // namespace

void JuSynthTest::test_noteOn_default_shouldPlayTheNotePitch()
{
    JuSynthDevice synth { "JU" };
    const auto buffer = render(synth);
    QVERIFY(rms(buffer) > 1.0e-3);
    // A saw at 8': the fundamental is the note, and stronger than anything half an octave off it
    QVERIFY(magnitudeAt(buffer, MiddleC) > 4.0 * magnitudeAt(buffer, MiddleC * std::sqrt(2.0)));
}

void JuSynthTest::test_range_16Feet_shouldPlayAnOctaveDown()
{
    JuSynthDevice synth { "JU" };
    synth.setRange(0);
    const auto buffer = render(synth);
    QVERIFY(magnitudeAt(buffer, MiddleC / 2.0) > 4.0 * magnitudeAt(buffer, MiddleC * 0.75));
}

void JuSynthTest::test_subLevel_shouldAddAnOctaveBelow()
{
    JuSynthDevice withoutSub { "JU" };
    const auto dry = render(withoutSub);

    JuSynthDevice withSub { "JU" };
    withSub.setSubLevel(1.0f);
    const auto sub = render(withSub);

    QVERIFY(magnitudeAt(dry, MiddleC / 2.0) < 1.0e-3);
    QVERIFY2(magnitudeAt(sub, MiddleC / 2.0) > 10.0 * magnitudeAt(dry, MiddleC / 2.0) + 1.0e-3,
             qPrintable(QString { "dry %1, with sub %2" }.arg(magnitudeAt(dry, MiddleC / 2.0)).arg(magnitudeAt(sub, MiddleC / 2.0))));
}

void JuSynthTest::test_pulseWidth_narrowed_shouldAddEvenHarmonics()
{
    // A square has no even harmonics; a narrow pulse has plenty
    const auto secondHarmonicShare = [](float pulseWidth) {
        JuSynthDevice synth { "JU" };
        synth.setSawEnabled(false);
        synth.setPulseEnabled(true);
        synth.setPwmSource(JuSynthDevice::PwmSource::Manual);
        synth.setPulseWidth(pulseWidth);
        const auto buffer = render(synth);
        return magnitudeAt(buffer, MiddleC * 2.0) / magnitudeAt(buffer, MiddleC);
    };
    const double square = secondHarmonicShare(0.0f);
    const double narrow = secondHarmonicShare(0.8f);
    QVERIFY2(square < 0.05, qPrintable(QString::number(square)));
    QVERIFY2(narrow > 0.3, qPrintable(QString::number(narrow)));
}

void JuSynthTest::test_lpfCutoff_lowered_shouldDarkenTheTone()
{
    const auto eighthHarmonic = [](float cutoff) {
        JuSynthDevice synth { "JU" };
        synth.setLpfCutoff(cutoff);
        return magnitudeAt(render(synth), MiddleC * 8.0);
    };
    // About 500 Hz against wide open
    QVERIFY(eighthHarmonic(0.47f) < 0.1 * eighthHarmonic(1.0f));
}

void JuSynthTest::test_lpfResonance_shouldBoostAroundTheCutoff()
{
    // Cutoff on the fourth harmonic, about 1050 Hz
    const auto fourthHarmonic = [](float resonance) {
        JuSynthDevice synth { "JU" };
        synth.setLpfCutoff(0.573f);
        synth.setLpfResonance(resonance);
        return magnitudeAt(render(synth), MiddleC * 4.0);
    };
    QVERIFY2(fourthHarmonic(0.8f) > 2.0 * fourthHarmonic(0.0f), qPrintable(QString { "%1 vs %2" }.arg(fourthHarmonic(0.8f)).arg(fourthHarmonic(0.0f))));
}

void JuSynthTest::test_hpfCutoff_raised_shouldThinTheBass()
{
    const auto fundamental = [](float hpf) {
        JuSynthDevice synth { "JU" };
        synth.setHpfCutoff(hpf);
        return magnitudeAt(render(synth), MiddleC);
    };
    // About 2 kHz
    QVERIFY(fundamental(0.67f) < 0.25 * fundamental(0.0f));
}

void JuSynthTest::test_chorusMode_off_shouldBeMono()
{
    JuSynthDevice synth { "JU" };
    const auto buffer = render(synth);
    for (size_t i = 0; i < buffer.size(); i += 2) {
        QCOMPARE(buffer[i], buffer[i + 1]);
    }
}

void JuSynthTest::test_chorusMode_on_shouldSpreadTheSides()
{
    for (auto && mode : { JunoChorus::Mode::I, JunoChorus::Mode::II, JunoChorus::Mode::Both }) {
        JuSynthDevice synth { "JU" };
        synth.setChorusMode(mode);
        const auto buffer = render(synth);
        double difference = 0.0;
        for (size_t i = 0; i < buffer.size(); i += 2) {
            difference += std::abs(buffer[i] - buffer[i + 1]);
        }
        QVERIFY(difference / (static_cast<double>(buffer.size()) / 2.0) > 0.1 * rms(buffer));
    }
}

void JuSynthTest::test_voices_seventhNote_shouldStealTheOldest()
{
    // Six notes held and a seventh: the first one goes, the rest keep sounding. Spread out, so
    // that no note sits close enough to another to be mistaken for it.
    JuSynthDevice synth { "JU" };
    for (auto && note : { 48, 52, 55, 59, 62, 65 }) {
        synth.processMidiNoteOn(static_cast<uint8_t>(note), 127);
    }
    synth.processMidiNoteOn(72, 127);
    std::vector<double> buffer(8192 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 8192, SampleRate };
    synth.processAudio(context);

    const auto noteHz = [](int note) { return 440.0 * std::exp2((note - 69) / 12.0); };
    QVERIFY(magnitudeAt(buffer, noteHz(72)) > 1.0e-3);
    QVERIFY(magnitudeAt(buffer, noteHz(65)) > 1.0e-3);
    QVERIFY2(magnitudeAt(buffer, noteHz(48)) < 0.05 * magnitudeAt(buffer, noteHz(52)),
             qPrintable(QString { "48: %1, 52: %2" }.arg(magnitudeAt(buffer, noteHz(48))).arg(magnitudeAt(buffer, noteHz(52)))));
}

void JuSynthTest::test_vcaMode_gate_shouldIgnoreTheEnvelope()
{
    const auto level = [](JuSynthDevice::VcaMode mode) {
        JuSynthDevice synth { "JU" };
        synth.setSustain(0.0f);
        synth.setDecay(0.1f);
        synth.setVcaMode(mode);
        return rms(render(synth, 60, 16384));
    };
    // With no sustain the envelope has died away; the gate holds the note at full level
    QVERIFY(level(JuSynthDevice::VcaMode::Envelope) < 0.01 * level(JuSynthDevice::VcaMode::Gate));
}

void JuSynthTest::test_settings_shouldRoundTrip()
{
    JuSynthDevice out { "JU" };
    out.setLfoRate(0.3f);
    out.setLfoDelay(0.4f);
    out.setRange(2);
    out.setPitchLfoDepth(0.2f);
    out.setPulseWidth(0.6f);
    out.setPwmSource(JuSynthDevice::PwmSource::Envelope);
    out.setSawEnabled(false);
    out.setPulseEnabled(true);
    out.setSubLevel(0.7f);
    out.setNoiseLevel(0.1f);
    out.setHpfCutoff(0.2f);
    out.setLpfCutoff(0.4f);
    out.setLpfResonance(0.5f);
    out.setEnvInverted(true);
    out.setEnvMod(0.3f);
    out.setFilterLfoDepth(0.25f);
    out.setKeyTrack(0.8f);
    out.setVcaMode(JuSynthDevice::VcaMode::Gate);
    out.setVelocitySensitivity(0.6f);
    out.setAttack(0.1f);
    out.setDecay(0.2f);
    out.setSustain(0.3f);
    out.setRelease(0.4f);
    out.setChorusMode(JunoChorus::Mode::Both);
    out.setPortamento(0.5f);
    out.setPitchBendRange(7);

    QString xml;
    {
        NahdXmlWriter writer { xml };
        out.serializeToXml(writer);
    }
    NahdXmlReader reader { xml };
    while (reader.readNextStartElement() && reader.name() != Constants::NahdXml::xmlKeyDevice()) {
    }
    JuSynthDevice in { "JU" };
    in.deserializeFromXml(reader);

    const auto near = [](float a, float b) { return std::abs(a - b) < 0.001f; };
    QVERIFY(near(in.lfoRate(), 0.3f));
    QVERIFY(near(in.lfoDelay(), 0.4f));
    QCOMPARE(in.range(), 2);
    QVERIFY(near(in.pitchLfoDepth(), 0.2f));
    QVERIFY(near(in.pulseWidth(), 0.6f));
    QCOMPARE(in.pwmSource(), JuSynthDevice::PwmSource::Envelope);
    QCOMPARE(in.sawEnabled(), false);
    QCOMPARE(in.pulseEnabled(), true);
    QVERIFY(near(in.subLevel(), 0.7f));
    QVERIFY(near(in.noiseLevel(), 0.1f));
    QVERIFY(near(in.hpfCutoff(), 0.2f));
    QVERIFY(near(in.lpfCutoff(), 0.4f));
    QVERIFY(near(in.lpfResonance(), 0.5f));
    QCOMPARE(in.envInverted(), true);
    QVERIFY(near(in.envMod(), 0.3f));
    QVERIFY(near(in.filterLfoDepth(), 0.25f));
    QVERIFY(near(in.keyTrack(), 0.8f));
    QCOMPARE(in.vcaMode(), JuSynthDevice::VcaMode::Gate);
    QVERIFY(near(in.velocitySensitivity(), 0.6f));
    QVERIFY(near(in.attack(), 0.1f));
    QVERIFY(near(in.decay(), 0.2f));
    QVERIFY(near(in.sustain(), 0.3f));
    QVERIFY(near(in.release(), 0.4f));
    QCOMPARE(in.chorusMode(), JunoChorus::Mode::Both);
    QVERIFY(near(in.portamento(), 0.5f));
    QCOMPARE(in.pitchBendRange(), 7);
}

void JuSynthTest::test_presets_shouldAllLoadAndBeNamedOnce()
{
    const auto & presets = JuSynthPresets::presets();
    QVERIFY(presets.size() > 10);
    QCOMPARE(presets.front().name, std::string { "Init" });

    std::set<std::string> names;
    for (int i = 0; i < static_cast<int>(presets.size()); i++) {
        const auto & preset = presets.at(static_cast<size_t>(i));
        QVERIFY2(names.insert(preset.name).second, preset.name.c_str());

        // Every key a preset names has to be a parameter, or it silently does nothing
        JuSynthDevice synth { "JU" };
        for (auto && [key, value] : preset.parameters) {
            QVERIFY2(synth.parameter(key).has_value(), (preset.name + ": " + key).c_str());
        }
        synth.loadPreset(i);
        QVERIFY2(rms(render(synth)) > 1.0e-4, preset.name.c_str());
    }
}

void JuSynthTest::test_programChange_shouldLoadThePreset()
{
    JuSynthDevice synth { "JU" };
    synth.processMidiProgramChange(1, 0);
    QCOMPARE(synth.chorusMode(), JunoChorus::Mode::II);
}

void JuSynthTest::test_automatableParameters_shouldOfferTheMainControls()
{
    const JuSynthDevice synth { "JU" };
    std::set<std::string> automatable;
    for (auto && entry : synth.automatableParameters()) {
        automatable.insert(entry.key);
    }
    namespace C = Constants::NahdXml;
    for (auto && key : { C::xmlKeyLpfCutoff(), C::xmlKeyLpfResonance(), C::xmlKeyHpfCutoff(), C::xmlKeyEnvMod(), C::xmlKeyPulseWidth(), C::xmlKeyLfoRate() }) {
        QVERIFY2(automatable.contains(key.toStdString()), qPrintable(key));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::JuSynthTest)
