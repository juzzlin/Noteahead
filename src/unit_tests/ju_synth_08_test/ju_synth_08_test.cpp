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

#include "ju_synth_08_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/ju_synth_08_device.hpp"
#include "../../domain/devices/ju_synth_08_presets.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QSignalSpy>
#include <QTest>
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

using namespace noteahead;

void JuSynth08Test::test_instantiation()
{
    JuSynth08Device synth { "Test JU Synth 08" };
    QCOMPARE(synth.name(), std::string("Test JU Synth 08"));
    QCOMPARE(synth.category(), std::string("Synth"));
    QVERIFY(!synth.typeId().empty());
    QVERIFY(!synth.typeName().empty());
    QVERIFY(!synth.hasActiveAudio());
}

void JuSynth08Test::test_polyModeNoteOnAndOff()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);

    synth.processDeviceMidiNoteOn(60, 100);
    QVERIFY(synth.hasActiveAudio());

    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };

    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);

    synth.processDeviceMidiNoteOff(60);
}

void JuSynth08Test::test_soloModeLegatoGlide()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setVoiceMode(static_cast<int>(JuSynth08Device::VoiceMode::Solo));
    synth.setPortamento(0.5f);

    synth.processDeviceMidiNoteOn(48, 100);
    std::vector<double> buffer(128 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 128,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    synth.processDeviceMidiNoteOn(60, 100); // legato glide
    std::fill(buffer.begin(), buffer.end(), 0.0);
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_unisonModeDetune()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setVoiceMode(static_cast<int>(JuSynth08Device::VoiceMode::Unison));
    synth.setUnisonDetune(0.5f);

    synth.processDeviceMidiNoteOn(60, 100);

    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_vcfSlopeSwitching()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);

    synth.setVcfSlope(static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));
    QCOMPARE(synth.vcfSlope(), static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));

    synth.setVcfSlope(static_cast<int>(JuSynth08Device::VcfSlope::Slope24dB));
    QCOMPARE(synth.vcfSlope(), static_cast<int>(JuSynth08Device::VcfSlope::Slope24dB));
}

void JuSynth08Test::test_vco2HardSync()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setSync(true);
    QVERIFY(synth.sync());

    synth.processDeviceMidiNoteOn(60, 100);
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_crossModulationFm()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);
    synth.setCrossMod(0.8f);
    QCOMPARE(synth.crossMod(), 0.8f);

    synth.processDeviceMidiNoteOn(60, 100);
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext ctx {
        .buffer = buffer,
        .frameCount = 256,
        .sampleRate = 44100,
    };
    synth.processAudio(ctx);

    const double energy = std::accumulate(buffer.begin(), buffer.end(), 0.0, [](double sum, double val) {
        return sum + std::abs(val);
    });
    QVERIFY(energy > 0.001);
}

void JuSynth08Test::test_presetLoadingDoesNotDeadlock()
{
    JuSynth08Device synth;
    synth.setSampleRate(44100.0);

    QSignalSpy spy { &synth, &Device::dataChanged };

    const auto presetCount = static_cast<int>(JuSynth08Presets::presets().size());
    QVERIFY(presetCount > 0);

    for (int i = 0; i < presetCount; i++) {
        synth.loadPreset(i);
    }

    QCOMPARE(spy.count(), presetCount);
}

void JuSynth08Test::test_advanceIdle_shouldKeepTheLfoRunning()
{
    const auto ledAfterSilence = [](bool idle) {
        JuSynth08Device synth { "JU08" };
        const auto rate = static_cast<uint32_t>(Constants::defaultSampleRate());
        constexpr uint32_t frames { 3000 };
        std::vector<double> silence(static_cast<size_t>(frames) * 2, 0.0);
        AudioContext context { .buffer = silence, .frameCount = frames, .sampleRate = rate };
        if (idle) {
            synth.advanceIdle(context);
        } else {
            synth.processAudio(context);
        }
        return synth.lfoLed(0);
    };
    const float rendered = ledAfterSilence(false);
    QVERIFY2(rendered > -0.99f, qPrintable(QString::number(rendered)));
    QVERIFY2(std::abs(ledAfterSilence(true) - rendered) < 1.0e-3f, qPrintable(QString::number(ledAfterSilence(true))));
}

namespace {

//! Holds @p note for @p seconds and returns the mono mix.
std::vector<double> renderHeldNote(JuSynth08Device & synth, uint8_t note, double seconds)
{
    constexpr uint32_t sampleRate { 48000 };
    constexpr size_t block { 256 };
    const auto blocks = static_cast<size_t>(seconds * sampleRate) / block;
    std::vector<double> mono;
    std::vector<double> buffer(block * 2);
    synth.processDeviceMidiNoteOn(note, 100);
    for (size_t i = 0; i < blocks; i++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { .buffer = buffer, .frameCount = block, .sampleRate = sampleRate };
        synth.processAudio(context);
        for (size_t frame = 0; frame < block; frame++) {
            mono.push_back(0.5 * (buffer[frame * 2] + buffer[frame * 2 + 1]));
        }
    }
    return mono;
}

double rms(const std::vector<double> & samples, size_t from, size_t to)
{
    double sum = 0.0;
    for (size_t i = from; i < to; i++) {
        sum += samples[i] * samples[i];
    }
    return std::sqrt(sum / static_cast<double>(to - from));
}

double peak(const std::vector<double> & samples)
{
    return std::ranges::max(samples, {}, [](double sample) { return std::abs(sample); });
}

//! Where the strongest partial within ten cents of @p hz sits, in cents from it, over the samples
//! from @p from to @p to.
double centsOff(const std::vector<double> & samples, size_t from, size_t to, double hz)
{
    const size_t count = to - from;
    double bestCents = 0.0;
    double bestMagnitude = 0.0;
    for (double cents = -10.0; cents <= 10.0; cents += 0.05) {
        const double probe = hz * std::exp2(cents / 1200.0);
        double re = 0.0;
        double im = 0.0;
        for (size_t i = 0; i < count; i++) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(count));
            const double phase = 2.0 * M_PI * probe * static_cast<double>(i) / 48000.0;
            re += window * samples[from + i] * std::cos(phase);
            im += window * samples[from + i] * std::sin(phase);
        }
        if (const double magnitude = std::hypot(re, im); magnitude > bestMagnitude) {
            bestMagnitude = magnitude;
            bestCents = cents;
        }
    }
    return bestCents;
}

//! How far a held C5 on VCO 1 alone wanders over four seconds, in cents from its lowest to its
//! highest, and the furthest it gets from concert pitch.
std::pair<double, double> wander(JuSynth08Device & synth)
{
    synth.setSampleRate(48000.0);
    synth.setVco2Level(0.0f);
    const auto mono = renderHeldNote(synth, 72, 4.0);
    constexpr size_t window { 12000 };
    double lowest = 100.0;
    double highest = -100.0;
    for (size_t from = 0; from + window <= mono.size(); from += window) {
        const double cents = centsOff(mono, from, from + window, 523.2511306);
        lowest = std::min(lowest, cents);
        highest = std::max(highest, cents);
    }
    return { highest - lowest, std::max(std::abs(lowest), std::abs(highest)) };
}

} // namespace

void JuSynth08Test::test_unisonNoteOn_shouldNotSpikeOnTheAttack()
{
    // Eight detuned voices started in step summed to a spike some 35 dB over the note they settle into
    JuSynth08Device synth;
    synth.setSampleRate(48000.0);
    synth.setVoiceMode(static_cast<int>(JuSynth08Device::VoiceMode::Unison));
    synth.setUnisonDetune(0.2f);
    const auto mono = renderHeldNote(synth, 48, 1.0);
    const double crest = std::abs(peak(mono)) / rms(mono, mono.size() / 2, mono.size());
    QVERIFY2(crest < 4.0, qPrintable(QString::number(crest)));
}

void JuSynth08Test::test_envMod_closedEnvelope_shouldLeaveTheCutoffAtTheKnob()
{
    // The envelope opens the filter from where the knob is: once it has decayed to nothing the
    // voice sounds as it does with no env mod at all, not two octaves darker
    const auto sustainedRms = [](float envMod) {
        JuSynth08Device synth;
        synth.setSampleRate(48000.0);
        synth.setLpfCutoff(0.5f);
        synth.setEnvMod(envMod);
        synth.setEnv1Attack(0.0f);
        synth.setEnv1Decay(0.0f);
        synth.setEnv1Sustain(0.0f);
        const auto mono = renderHeldNote(synth, 48, 1.0);
        return rms(mono, mono.size() / 2, mono.size());
    };
    const double db = 20.0 * std::log10(sustainedRms(0.5f) / sustainedRms(0.0f));
    QVERIFY2(std::abs(db) < 1.0, qPrintable(QString::number(db)));
}

void JuSynth08Test::test_presets_heldNote_shouldBeAudibleAndUnclipped()
{
    const auto & presets = JuSynth08Presets::presets();
    for (size_t i = 0; i < presets.size(); i++) {
        JuSynth08Device synth;
        synth.setSampleRate(48000.0);
        synth.loadPreset(static_cast<int>(i));
        const double db = 20.0 * std::log10(std::abs(peak(renderHeldNote(synth, 60, 1.0))));
        QVERIFY2(db > -40.0 && db < -3.0, qPrintable(QString::fromStdString(presets[i].name) + ": " + QString::number(db)));
    }
}

void JuSynth08Test::test_initPreset_heldNote_shouldPlayAtConcertPitch()
{
    // Both bipolar controls sit at half travel after a reset. Read as unipolar, that put the pitch
    // envelope at +12 semitones and VCO 2 25 cents sharp on every preset.
    JuSynth08Device synth { "p" };
    synth.loadPreset(0);
    const auto mono = renderHeldNote(synth, 69, 2.0);
    const size_t from = mono.size() / 2;
    size_t rises = 0;
    for (size_t i = from + 1; i < mono.size(); i++) {
        rises += mono[i - 1] < 0.0 && mono[i] >= 0.0;
    }
    const double hz = static_cast<double>(rises) * 48000.0 / static_cast<double>(mono.size() - from);
    QVERIFY2(std::abs(hz - 440.0) < 2.0, qPrintable(QString::number(hz)));
}

void JuSynth08Test::test_oscillatorDrift_zero_shouldHoldThePitch()
{
    JuSynth08Device synth;
    const auto [spread, furthest] = wander(synth);
    QVERIFY2(spread < 0.2, qPrintable(QString::number(spread)));
    QVERIFY2(furthest < 0.2, qPrintable(QString::number(furthest)));
}

void JuSynth08Test::test_oscillatorDrift_full_shouldWanderAFewCents()
{
    JuSynth08Device synth;
    synth.setOscillatorDrift(1.0f);
    const auto [spread, furthest] = wander(synth);
    QVERIFY2(spread > 2.0, qPrintable(QString::number(spread)));
    QVERIFY2(furthest < 5.5, qPrintable(QString::number(furthest)));
}

void JuSynth08Test::test_oscillatorInstability_full_shouldWanderAFewCents()
{
    JuSynth08Device synth;
    synth.setOscillatorInstability(1.0f);
    const auto [spread, furthest] = wander(synth);
    QVERIFY2(spread > 2.0, qPrintable(QString::number(spread)));
    QVERIFY2(furthest < 3.5, qPrintable(QString::number(furthest)));
}

void JuSynth08Test::test_oscillatorInstability_full_shouldBeatTheTwoVcos()
{
    // The two VCOs at one pitch sum to a level that never moves, until each wanders on its own
    const auto levelSpread = [](float instability) {
        JuSynth08Device synth;
        synth.setSampleRate(48000.0);
        synth.setOscillatorInstability(instability);
        const auto mono = renderHeldNote(synth, 60, 4.0);
        double lowest = 1.0;
        double highest = 0.0;
        for (size_t from = 24000; from + 4800 <= mono.size(); from += 4800) {
            const double level = rms(mono, from, from + 4800);
            lowest = std::min(lowest, level);
            highest = std::max(highest, level);
        }
        return 20.0 * std::log10(highest / lowest);
    };
    const double steady = levelSpread(0.0f);
    QVERIFY2(steady < 0.1, qPrintable(QString::number(steady)));
    const double beating = levelSpread(1.0f);
    QVERIFY2(beating > 1.0, qPrintable(QString::number(beating)));
}

void JuSynth08Test::test_oscillatorDrift_afterReset_shouldRenderTheSame()
{
    // As on the Synth: a bounce is repeatable, whatever was played before it
    const auto configure = [](JuSynth08Device & synth) {
        synth.setSampleRate(48000.0);
        synth.setOscillatorDrift(1.0f);
        synth.setOscillatorInstability(1.0f);
        synth.setVoiceTolerance(1.0f);
    };
    JuSynth08Device synth;
    configure(synth);
    const auto first = renderHeldNote(synth, 60, 1.0);
    synth.resetAudio();
    QVERIFY(renderHeldNote(synth, 60, 1.0) == first);
    JuSynth08Device other;
    configure(other);
    QVERIFY(renderHeldNote(other, 60, 1.0) == first);
}

void JuSynth08Test::test_voiceTolerance_full_shouldTuneTheVoicesApart()
{
    // C5 on the first voice alone, then on the second with C#4 holding the first: no partial of
    // C#4 comes within ten cents of C5
    const auto offsets = [](float tolerance) {
        std::array<double, 2> cents {};
        for (size_t slot = 0; slot < cents.size(); slot++) {
            JuSynth08Device synth;
            synth.setSampleRate(48000.0);
            synth.setVco2Level(0.0f);
            synth.setVoiceTolerance(tolerance);
            if (slot == 1) {
                synth.processDeviceMidiNoteOn(61, 100);
            }
            const auto mono = renderHeldNote(synth, 72, 0.5);
            cents[slot] = centsOff(mono, mono.size() / 4, mono.size(), 523.2511306);
        }
        return cents;
    };
    const auto exact = offsets(0.0f);
    QVERIFY2(std::abs(exact[0]) < 0.2 && std::abs(exact[1]) < 0.2, qPrintable(QString::number(exact[0]) + " " + QString::number(exact[1])));
    const auto apart = offsets(1.0f);
    QVERIFY2(std::abs(apart[0] - apart[1]) > 2.0, qPrintable(QString::number(apart[0]) + " " + QString::number(apart[1])));
    QVERIFY2(std::abs(apart[0]) < 5.5 && std::abs(apart[1]) < 5.5, qPrintable(QString::number(apart[0]) + " " + QString::number(apart[1])));
}

void JuSynth08Test::test_xmlSerialization()
{
    JuSynth08Device synth { "JuSynth08Source" };
    synth.setVcfSlope(static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));
    synth.setCrossMod(0.65f);
    synth.setUnisonDetune(0.42f);
    synth.setOscillatorDrift(0.35f);
    synth.setOscillatorInstability(0.45f);
    synth.setVoiceTolerance(0.55f);

    QString xml;
    {
        NahdXmlWriter writer { xml };
        synth.serializeToXml(writer);
    }

    JuSynth08Device restored { "JuSynth08Restored" };
    NahdXmlReader reader { xml };
    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevice());
    restored.deserializeFromXml(reader);

    QCOMPARE(restored.vcfSlope(), static_cast<int>(JuSynth08Device::VcfSlope::Slope12dB));
    QCOMPARE(restored.crossMod(), 0.65f);
    QCOMPARE(restored.unisonDetune(), 0.42f);
    QCOMPARE(restored.oscillatorDrift(), 0.35f);
    QCOMPARE(restored.oscillatorInstability(), 0.45f);
    QCOMPARE(restored.voiceTolerance(), 0.55f);
}

QTEST_GUILESS_MAIN(JuSynth08Test)
