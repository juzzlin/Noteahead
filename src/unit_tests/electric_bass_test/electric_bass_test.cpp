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

#include "electric_bass_test.hpp"

#include "../../application/service/electric_bass_preview.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/electric_bass_device.hpp"
#include "../../domain/devices/electric_bass_presets.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/modal_bass_string.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

constexpr double SampleRate = 44100.0;

double noteHz(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

std::vector<double> render(ModalBassString & string, double seconds)
{
    std::vector<double> out;
    for (size_t i = 0; i < static_cast<size_t>(seconds * SampleRate); i++) {
        out.push_back(string.nextSample());
    }
    return out;
}

//! The pitch over [from, to) seconds, from the spacing of the upward zero crossings of the signal
//! low-passed to its fundamental
double pitchHz(const std::vector<double> & x, double from, double to, double nominalHz)
{
    // Two one-pole low-passes at the fundamental leave little but it to cross zero
    const double a = 1.0 - std::exp(-2.0 * M_PI * nominalHz / SampleRate);
    double s1 = 0.0;
    double s2 = 0.0;
    std::vector<double> y;
    for (auto && v : x) {
        s1 += a * (v - s1);
        s2 += a * (s1 - s2);
        y.push_back(s2);
    }
    std::vector<double> crossings;
    for (size_t i = static_cast<size_t>(from * SampleRate) + 1; i < std::min(y.size(), static_cast<size_t>(to * SampleRate)); i++) {
        if (y.at(i - 1) < 0.0 && y.at(i) >= 0.0) {
            crossings.push_back(static_cast<double>(i - 1) + y.at(i - 1) / (y.at(i - 1) - y.at(i)));
        }
    }
    return crossings.size() < 2 ? 0.0 : SampleRate * static_cast<double>(crossings.size() - 1) / (crossings.back() - crossings.front());
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

double peak(const std::vector<double> & x)
{
    double p = 0.0;
    for (auto && v : x) {
        p = std::max(p, std::abs(v));
    }
    return p;
}

void renderDevice(ElectricBassDevice & device, double seconds, std::vector<double> & left)
{
    const auto frames = static_cast<uint32_t>(seconds * SampleRate);
    std::vector<double> block(512 * 2);
    for (uint32_t done = 0; done < frames; done += 512) {
        std::ranges::fill(block, 0.0);
        AudioContext context { .buffer = block, .frameCount = 512, .sampleRate = static_cast<uint32_t>(SampleRate) };
        device.processAudio(context);
        for (size_t i = 0; i < 512; i++) {
            left.push_back(block.at(i * 2));
        }
    }
}

} // namespace

void ElectricBassTest::test_string_pluck_shouldSoundAtItsPitchAndDecay()
{
    ModalBassString string;
    string.setSampleRate(SampleRate);
    string.pluck(noteHz(28), 0.0, 2.6e-4, 1.0, 0.5);
    const auto x = render(string, 3.0);
    QVERIFY(std::abs(pitchHz(x, 0.5, 2.5, noteHz(28)) - noteHz(28)) < 0.3);
    QVERIFY(rms(x, 0.0, 0.2) > 0.0);
    QVERIFY(rms(x, 2.5, 3.0) < rms(x, 0.1, 0.6));
    QVERIFY(string.isHeld());
}

void ElectricBassTest::test_string_mute_shouldDieFasterThanHeld()
{
    ModalBassString held;
    ModalBassString muted;
    for (auto && string : { &held, &muted }) {
        string->setSampleRate(SampleRate);
        string->pluck(noteHz(33), 0.0, 2.2e-4, 1.0, 0.5);
        render(*string, 0.2);
    }
    muted.mute();
    const auto h = render(held, 0.5);
    const auto m = render(muted, 0.5);
    // The default release is about a tenth of a second: half a second later the note is 40 dB down
    QVERIFY(rms(m, 0.4, 0.5) < rms(h, 0.4, 0.5) * 0.01);
    QVERIFY(!muted.isHeld());
}

void ElectricBassTest::test_string_pitchRatio_shouldBendWithoutAJump()
{
    ModalBassString string;
    string.setSampleRate(SampleRate);
    string.pluck(noteHz(40), 12.0, 2.6e-4, 1.0, 0.5);
    auto x = render(string, 0.5);
    const double before = std::abs(x.back() - x.at(x.size() - 2));
    string.setPitchRatio(std::exp2(2.0 / 12.0));
    const auto y = render(string, 1.0);
    // The partials only turn faster: the first sample after the bend continues the waveform
    QVERIFY(std::abs(y.front() - x.back()) < before * 3.0 + 1.0e-6);
    QVERIFY(std::abs(pitchHz(y, 0.2, 1.0, noteHz(42)) - noteHz(42)) < 1.0);
}

void ElectricBassTest::test_device_noteOn_shouldTakeTheLowestFret()
{
    ElectricBassDevice device { "Bass" };
    // A1 is the open A, not the fifth fret of the E or the tenth of the B
    device.processMidiNoteOn(33, 100);
    QCOMPARE(device.stringOf(33), std::optional<size_t> { 2 });
    device.processMidiNoteOff(33);
    // C3 is the fifth fret of the G, the lowest fret that reaches it
    device.processMidiNoteOn(48, 100);
    QCOMPARE(device.stringOf(48), std::optional<size_t> { 4 });
    device.processMidiNoteOff(48);
    // B0 is only on the B
    device.processMidiNoteOn(23, 100);
    QCOMPARE(device.stringOf(23), std::optional<size_t> { 0 });
}

void ElectricBassTest::test_device_noteOn_repeated_shouldReuseItsString()
{
    ElectricBassDevice device { "Bass" };
    device.processMidiNoteOn(40, 100);
    const auto first = device.stringOf(40);
    device.processMidiNoteOff(40);
    device.processMidiNoteOn(40, 100);
    QCOMPARE(device.stringOf(40), first);
}

void ElectricBassTest::test_device_noteOn_chord_shouldUseSeparateStrings()
{
    ElectricBassDevice device { "Bass" };
    // An octave: too far for a hammer-on, so a double stop on two strings
    device.processMidiNoteOn(40, 100);
    device.processMidiNoteOn(52, 100);
    QVERIFY(device.stringOf(40).has_value());
    QVERIFY(device.stringOf(52).has_value());
    QVERIFY(device.stringOf(40) != device.stringOf(52));
}

void ElectricBassTest::test_device_noteOn_overlapping_shouldPlayLegatoOnTheSameString()
{
    ElectricBassDevice device { "Bass" };
    device.processMidiNoteOn(40, 100);
    const auto string = device.stringOf(40);
    device.processMidiNoteOn(42, 100);
    QCOMPARE(device.stringOf(42), string);
    QVERIFY(!device.stringOf(40).has_value());
}

void ElectricBassTest::test_device_noteOff_legato_shouldPullOffToTheHeldNote()
{
    ElectricBassDevice device { "Bass" };
    device.processMidiNoteOn(40, 100);
    device.processMidiNoteOn(42, 100);
    device.processMidiNoteOff(42);
    QVERIFY(device.stringOf(40).has_value());

    std::vector<double> x;
    renderDevice(device, 1.0, x);
    QVERIFY(std::abs(pitchHz(x, 0.3, 1.0, noteHz(40)) - noteHz(40)) < 1.0);
}

void ElectricBassTest::test_device_pitchBend_shouldRaiseThePitch()
{
    ElectricBassDevice device { "Bass" };
    device.processMidiNoteOn(40, 100);
    device.processMidiPitchBend(16383, 0);
    std::vector<double> x;
    renderDevice(device, 1.0, x);
    // Two semitones up by default
    QVERIFY(std::abs(pitchHz(x, 0.3, 1.0, noteHz(42)) - noteHz(40) * std::exp2(2.0 * 8191.0 / 8192.0 / 12.0)) < 1.0);
}

void ElectricBassTest::test_device_octave_up_shouldPlayAnOctaveHigher()
{
    ElectricBassDevice device { "Bass" };
    device.setOctave(1);
    device.processMidiNoteOn(40, 100);
    std::vector<double> x;
    renderDevice(device, 1.0, x);
    QVERIFY(std::abs(pitchHz(x, 0.3, 1.0, noteHz(52)) - noteHz(52)) < 1.0);
}

void ElectricBassTest::test_device_lpfCutoff_closed_shouldQuietenTheNote()
{
    const auto level = [](float lpf) {
        ElectricBassDevice device { "Bass" };
        device.setKnob(Xml::xmlKeyLpfCutoff(), lpf);
        device.processMidiNoteOn(40, 127);
        std::vector<double> x;
        renderDevice(device, 0.5, x);
        return 20.0 * std::log10(peak(x));
    };
    QVERIFY(level(0.0f) < level(1.0f) - 10.0);
}

void ElectricBassTest::test_presets_every_shouldPeakNearInit()
{
    double initPeak = 0.0;
    for (size_t i = 0; i < ElectricBassPresets::presets().size(); i++) {
        ElectricBassDevice device { "Bass" };
        device.loadPreset(static_cast<int>(i));
        device.processMidiNoteOn(28, 127);
        std::vector<double> x;
        renderDevice(device, 1.0, x);
        const double level = 20.0 * std::log10(peak(x));
        if (i == 0) {
            initPeak = level;
            // A full-strength open E, where OutputGain puts it
            QVERIFY(std::abs(level + 8.0) < 1.0);
        }
        QVERIFY2(std::abs(level - initPeak) < 1.5, ElectricBassPresets::presets().at(i).name.c_str());
    }
}

void ElectricBassTest::test_preview_render_shouldSpanTheHoldAndTheRelease()
{
    const ElectricBassDevice device { "Bass" };
    const auto preview = ElectricBassPreview::render(device, 200, SampleRate);
    QCOMPARE(preview.peaks.size(), 200);
    // Two seconds held, then the release, which at the default dies well inside its limit
    QVERIFY(preview.durationSeconds > 2.0);
    QVERIFY(preview.durationSeconds < 3.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ElectricBassTest)
