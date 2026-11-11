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

#include "synth_d_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/dsp/synth_d_voice.hpp"
#include "../../synth_d_device.hpp"
#include "../../synth_d_presets.hpp"

#include <QTest>

#include <cmath>
#include <complex>
#include <vector>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;
using Voice = SynthDVoice;

constexpr double SampleRate = 48000.0;

struct Stereo
{
    std::vector<double> left;
    std::vector<double> right;

    std::vector<double> mono() const
    {
        std::vector<double> out(left.size());
        for (size_t i = 0; i < left.size(); i++) {
            out.at(i) = 0.5 * (left.at(i) + right.at(i));
        }
        return out;
    }
};

void render(SynthDDevice & device, double seconds, Stereo & out)
{
    const auto blocks = static_cast<int>(seconds * SampleRate / 512.0);
    std::vector<double> block(512 * 2);
    for (int b = 0; b < blocks; b++) {
        std::ranges::fill(block, 0.0);
        AudioContext context { .buffer = block, .frameCount = 512, .sampleRate = static_cast<uint32_t>(SampleRate) };
        device.processAudio(context);
        for (size_t i = 0; i < 512; i++) {
            out.left.push_back(block.at(i * 2));
            out.right.push_back(block.at(i * 2 + 1));
        }
    }
}

//! A device with the analogue wander and tolerances off, so that it plays exactly in tune
std::unique_ptr<SynthDDevice> exactDevice()
{
    auto device = std::make_unique<SynthDDevice>("Synth D");
    device->setKnob(Xml::xmlKeyOscillatorDrift(), 0.0f);
    device->setKnob(Xml::xmlKeyOscillatorInstability(), 0.0f);
    device->setKnob(Xml::xmlKeyVoiceTolerance(), 0.0f);
    return device;
}

double rms(const std::vector<double> & x, double from, double to)
{
    double sum = 0.0;
    const auto a = static_cast<size_t>(from * SampleRate);
    const auto b = std::min(x.size(), static_cast<size_t>(to * SampleRate));
    for (size_t i = a; i < b; i++) {
        sum += x.at(i) * x.at(i);
    }
    return b > a ? std::sqrt(sum / static_cast<double>(b - a)) : 0.0;
}

//! The loudest 50 ms of the first @p seconds: a percussive patch is judged by its hit, not its tail
double loudestDb(const std::vector<double> & x, double seconds)
{
    double loudest = 0.0;
    for (double t = 0.0; t + 0.05 <= seconds; t += 0.025) {
        loudest = std::max(loudest, rms(x, t, t + 0.05));
    }
    return 20.0 * std::log10(std::max(loudest, 1.0e-12));
}

double db(double value)
{
    return 20.0 * std::log10(std::max(value, 1.0e-12));
}

//! The strongest frequency within 3 % of @p nominalHz over the given stretch
double peakHz(const std::vector<double> & x, double nominalHz, double from, double to)
{
    const auto a = static_cast<size_t>(from * SampleRate);
    const auto b = std::min(x.size(), static_cast<size_t>(to * SampleRate));
    const auto n = b - a;
    double bestHz = nominalHz;
    double best = 0.0;
    for (double hz = nominalHz * 0.97; hz <= nominalHz * 1.03; hz += nominalHz * 2.0e-4) {
        std::complex<double> sum {};
        const double w = 2.0 * M_PI * hz / SampleRate;
        for (size_t i = 0; i < n; i += 2) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(n));
            sum += x.at(a + i) * window * std::polar(1.0, -w * static_cast<double>(i));
        }
        if (std::abs(sum) > best) {
            best = std::abs(sum);
            bestHz = hz;
        }
    }
    return bestHz;
}

double cents(double hz, double reference)
{
    return 1200.0 * std::log2(hz / reference);
}

//! A patch whose loudness contour decays to silence in a tenth of a second and stays there
void percussive(SynthDDevice & device)
{
    device.setKnob(Xml::xmlKeyEnv2Decay(), 0.15f);
    device.setKnob(Xml::xmlKeyEnv2Sustain(), 0.0f);
}

} // namespace

void SynthDTest::test_initPreset_heldNote_shouldPlayAtConcertPitch()
{
    auto device = exactDevice();
    device->setKnob(Xml::xmlKeyCutoff(), 0.9f);
    device->processMidiNoteOn(69, 100);
    Stereo out;
    render(*device, 0.6, out);
    const double hz = peakHz(out.mono(), 440.0, 0.1, 0.6);
    QVERIFY2(std::abs(cents(hz, 440.0)) < 3.0, qPrintable(QString::number(hz)));
}

void SynthDTest::test_range_feet16_shouldPlayAnOctaveDown()
{
    auto device = exactDevice();
    device->setKnob(Xml::xmlKeyCutoff(), 0.9f);
    device->setChoice(Xml::xmlKeyVco1Range(), static_cast<int>(Voice::Range::Feet16));
    device->processMidiNoteOn(69, 100);
    Stereo out;
    render(*device, 0.6, out);
    const double hz = peakHz(out.mono(), 220.0, 0.1, 0.6);
    QVERIFY2(std::abs(cents(hz, 220.0)) < 3.0, qPrintable(QString::number(hz)));
}

void SynthDTest::test_osc3_keyboardControlOff_shouldIgnoreTheKey()
{
    for (const uint8_t note : { 48, 72 }) {
        auto device = exactDevice();
        device->setKnob(Xml::xmlKeyCutoff(), 0.9f);
        device->setChoice(Xml::xmlKeyVco1On(), 0);
        device->setChoice(Xml::xmlKeyVco3On(), 1);
        device->setChoice(Xml::xmlKeyVco3KeyboardControl(), 0);
        device->processMidiNoteOn(note, 100);
        Stereo out;
        render(*device, 0.6, out);
        const double expected = Voice::oscillatorHz(Voice::Range::Feet8, Voice::Osc3FixedNote);
        const double hz = peakHz(out.mono(), expected, 0.1, 0.6);
        QVERIFY2(std::abs(cents(hz, expected)) < 3.0, qPrintable(QString::number(hz)));
    }
}

void SynthDTest::test_timeKnobs_panelMarks_shouldMatchThePanel()
{
    QCOMPARE(Voice::attackSeconds(0.0), 0.001);
    QVERIFY(std::abs(Voice::attackSeconds(0.2) - 0.2) < 1.0e-9);
    QVERIFY(std::abs(Voice::attackSeconds(0.6) - 1.0) < 1.0e-9);
    QVERIFY(std::abs(Voice::attackSeconds(1.0) - 10.0) < 1.0e-9);
    QVERIFY(std::abs(Voice::decaySeconds(0.0) - 0.004) < 1.0e-9);
    QVERIFY(std::abs(Voice::decaySeconds(1.0) - 35.0) < 1.0e-9);
}

void SynthDTest::test_notePriority_low_shouldKeepTheLowestKey()
{
    SynthDDevice device { "Synth D" };
    device.processMidiNoteOn(60, 100);
    device.processMidiNoteOn(64, 100);
    QCOMPARE(device.soundingNote(), std::optional<uint8_t> { 60 });
    device.processMidiNoteOn(55, 100);
    QCOMPARE(device.soundingNote(), std::optional<uint8_t> { 55 });
    device.processMidiNoteOff(55);
    device.processMidiNoteOff(60);
    QCOMPARE(device.soundingNote(), std::optional<uint8_t> { 64 });
    device.processMidiNoteOff(64);
    QVERIFY(!device.soundingNote());
}

void SynthDTest::test_notePriority_last_shouldFollowTheNewestKey()
{
    SynthDDevice device { "Synth D" };
    device.setChoice(Xml::xmlKeyNotePriority(), static_cast<int>(SynthDDevice::NotePriority::Last));
    device.processMidiNoteOn(60, 100);
    device.processMidiNoteOn(64, 100);
    QCOMPARE(device.soundingNote(), std::optional<uint8_t> { 64 });
    device.processMidiNoteOff(64);
    QCOMPARE(device.soundingNote(), std::optional<uint8_t> { 60 });
}

void SynthDTest::test_multiTrigger_off_shouldNotRestartTheContours()
{
    auto device = exactDevice();
    percussive(*device);
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.5, out);
    // Legato: the second key moves the pitch, but the contours have run their course
    device->processMidiNoteOn(43, 100);
    render(*device, 0.3, out);
    QVERIFY2(db(rms(out.mono(), 0.55, 0.65)) < db(rms(out.mono(), 0.0, 0.05)) - 40.0, "a held key restarted the contours");
}

void SynthDTest::test_multiTrigger_on_shouldRestartTheContours()
{
    auto device = exactDevice();
    percussive(*device);
    device->setChoice(Xml::xmlKeyRetrigger(), 1);
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.5, out);
    device->processMidiNoteOn(43, 100);
    render(*device, 0.3, out);
    QVERIFY2(db(rms(out.mono(), 0.5, 0.55)) > db(rms(out.mono(), 0.0, 0.05)) - 6.0, "the new key did not restart the contours");
}

void SynthDTest::test_decaySwitch_off_shouldStopAtOnce()
{
    auto device = exactDevice();
    device->setKnob(Xml::xmlKeyEnv2Decay(), 0.6f);
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.3, out);
    device->processMidiNoteOff(48);
    render(*device, 0.3, out);
    QVERIFY(db(rms(out.mono(), 0.35, 0.4)) < db(rms(out.mono(), 0.2, 0.25)) - 60.0);
    QVERIFY(!device->hasActiveAudio());
}

void SynthDTest::test_decaySwitch_on_shouldRingForTheDecayTime()
{
    auto device = exactDevice();
    // A one-second decay at 6
    device->setKnob(Xml::xmlKeyEnv2Decay(), 0.6f);
    device->setChoice(Xml::xmlKeyEnv2DecaySwitch(), 1);
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.3, out);
    device->processMidiNoteOff(48);
    render(*device, 1.5, out);
    const double held = db(rms(out.mono(), 0.2, 0.25));
    const double half = db(rms(out.mono(), 0.75, 0.8));
    const double after = db(rms(out.mono(), 1.5, 1.55));
    QVERIFY2(half < held - 10.0 && half > held - 30.0, qPrintable(QString::number(half - held)));
    QVERIFY2(after < held - 40.0, qPrintable(QString::number(after - held)));
}

void SynthDTest::test_emphasis_full_shouldSingWithEverySourceOff()
{
    auto device = exactDevice();
    device->setChoice(Xml::xmlKeyVco1On(), 0);
    device->setKnob(Xml::xmlKeyResonance(), 1.0f);
    device->setKnob(Xml::xmlKeyEnvMod(), 0.0f);
    device->processMidiNoteOn(60, 100);
    Stereo out;
    render(*device, 1.5, out);
    QVERIFY2(db(rms(out.mono(), 1.0, 1.5)) > -40.0, qPrintable(QString::number(db(rms(out.mono(), 1.0, 1.5)))));
}

void SynthDTest::test_mixer_threeLoudOscillators_shouldLightTheOverloadLed()
{
    auto device = exactDevice();
    device->setChoice(Xml::xmlKeyVco2On(), 1);
    device->setChoice(Xml::xmlKeyVco3On(), 1);
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.2, out);
    QVERIFY(device->lfoLed(SynthDDevice::OverloadLed) > 0.5f);
}

void SynthDTest::test_mixer_oneOscillator_shouldLeaveTheOverloadLedDark()
{
    auto device = exactDevice();
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.2, out);
    QVERIFY(device->lfoLed(SynthDDevice::OverloadLed) < -0.5f);
}

void SynthDTest::test_lfo_idle_shouldKeepTheLedRunning()
{
    auto device = exactDevice();
    std::vector<double> block(512 * 2);
    float lowest = 1.0f;
    float highest = -1.0f;
    for (int i = 0; i < static_cast<int>(SampleRate / 512.0); i++) {
        AudioContext context { .buffer = block, .frameCount = 512, .sampleRate = static_cast<uint32_t>(SampleRate) };
        device->advanceIdle(context);
        lowest = std::min(lowest, device->lfoLed(SynthDDevice::LfoLed));
        highest = std::max(highest, device->lfoLed(SynthDDevice::LfoLed));
    }
    QVERIFY(lowest < -0.8f);
    QVERIFY(highest > 0.8f);
}

void SynthDTest::test_lfo_filterModulation_shouldSweepTheCutoff()
{
    auto device = exactDevice();
    device->setKnob(Xml::xmlKeyCutoff(), 0.3f);
    device->setKnob(Xml::xmlKeyEnvMod(), 0.0f);
    device->setChoice(Xml::xmlKeyModSource2(), static_cast<int>(Voice::ModSource2::Lfo));
    device->setChoice(Xml::xmlKeyLfoWaveform(), static_cast<int>(Voice::LfoWaveform::Square));
    device->setChoice(Xml::xmlKeyFilterModulation(), 1);
    device->setKnob(Xml::xmlKeyModMix(), 1.0f);
    device->setKnob(Xml::xmlKeyModDepth(), 1.0f);
    device->setKnob(Xml::xmlKeyLfoRate(), static_cast<float>(std::log(1.0 / 0.05) / std::log(4000.0))); // 1 Hz
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 1.0, out);
    const double open = db(rms(out.mono(), 0.1, 0.4));
    const double closed = db(rms(out.mono(), 0.6, 0.9));
    QVERIFY2(open - closed > 6.0, qPrintable(QString("%1 %2").arg(open).arg(closed)));
}

void SynthDTest::test_feedback_full_shouldStayBounded()
{
    auto device = exactDevice();
    device->setChoice(Xml::xmlKeyVco2On(), 1);
    device->setChoice(Xml::xmlKeyVco3On(), 1);
    device->setChoice(Xml::xmlKeyFeedbackOn(), 1);
    device->setKnob(Xml::xmlKeyFeedback(), 1.0f);
    device->setKnob(Xml::xmlKeyResonance(), 1.0f);
    device->processMidiNoteOn(36, 127);
    Stereo out;
    render(*device, 1.0, out);
    for (auto && v : out.left) {
        QVERIFY(std::isfinite(v));
        QVERIFY(std::abs(v) < 2.0);
    }
}

void SynthDTest::test_panSpread_zero_shouldBeMono()
{
    auto device = exactDevice();
    device->setChoice(Xml::xmlKeyVco2On(), 1);
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.3, out);
    // The output panner at centre is cos and sin of the same angle, which differ in the last bit
    for (size_t i = 0; i < out.left.size(); i++) {
        QVERIFY(std::abs(out.left.at(i) - out.right.at(i)) < 1.0e-12);
    }
}

void SynthDTest::test_panSpread_full_shouldPutOsc1Left()
{
    auto device = exactDevice();
    device->setKnob(Xml::xmlKeyPanSpread(), 1.0f);
    device->processMidiNoteOn(48, 100);
    Stereo out;
    render(*device, 0.3, out);
    QVERIFY(db(rms(out.right, 0.1, 0.3)) < db(rms(out.left, 0.1, 0.3)) - 60.0);
}

void SynthDTest::test_presets_all_shouldPlayAtSimilarLevels()
{
    const auto & presets = SynthDPresets::presets();
    double initDb = 0.0;
    QString levels;
    bool ok = true;
    for (size_t i = 0; i < presets.size(); i++) {
        SynthDDevice device { "Synth D" };
        device.loadPreset(static_cast<int>(i));
        device.processMidiNoteOn(48, 100);
        Stereo out;
        render(device, 0.8, out);
        device.processMidiNoteOff(48);
        render(device, 0.4, out);
        const double level = loudestDb(out.mono(), 0.8);
        if (i == 0) {
            initDb = level;
        }
        levels += QString { "%1 %2 dB\n" }.arg(QString::fromStdString(presets.at(i).name)).arg(level - initDb, 0, 'f', 1);
        ok &= std::abs(level - initDb) < 4.0;
    }
    QVERIFY2(ok, qPrintable(levels));
}

void SynthDTest::test_presets_all_shouldStayFinite()
{
    const auto & presets = SynthDPresets::presets();
    for (size_t i = 0; i < presets.size(); i++) {
        SynthDDevice device { "Synth D" };
        device.loadPreset(static_cast<int>(i));
        Stereo out;
        for (const uint8_t note : { 36, 60, 84 }) {
            device.processMidiNoteOn(note, 127);
            render(device, 0.3, out);
            device.processMidiNoteOff(note);
            render(device, 0.1, out);
        }
        for (auto && v : out.left) {
            QVERIFY(std::isfinite(v));
            QVERIFY2(std::abs(v) < 2.0, presets.at(i).name.c_str());
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SynthDTest)
