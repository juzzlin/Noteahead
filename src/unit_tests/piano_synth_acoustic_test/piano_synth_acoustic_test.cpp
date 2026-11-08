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

#include "piano_synth_acoustic_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/piano_synth_acoustic_device.hpp"
#include "../../domain/devices/piano_synth_acoustic_presets.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/modal_grand_string.hpp"

#include <QTest>

#include <cmath>
#include <complex>
#include <vector>

namespace noteahead {

namespace {

namespace Xml = Constants::NahdXml;

constexpr double SampleRate = 48000.0;

//! The recording's peak level per key, every third key from A#0, against its own mean
constexpr std::array<std::pair<int, double>, 30> ReferencePeaks { { { 22, -21.6 }, { 25, -22.4 }, { 28, -22.9 }, { 31, -22.9 }, { 34, -21.9 }, { 37, -23.4 }, { 40, -23.9 }, { 43, -23.9 }, { 46, -23.3 }, { 49, -23.3 }, { 52, -23.5 }, { 55, -23.6 }, { 58, -23.6 }, { 61, -22.8 }, { 64, -21.5 }, { 67, -21.5 }, { 70, -22.0 }, { 73, -22.8 }, { 76, -21.5 }, { 79, -21.5 }, { 82, -20.7 }, { 85, -21.9 }, { 88, -22.9 }, { 91, -22.9 }, { 94, -20.2 }, { 97, -22.8 }, { 100, -24.3 }, { 103, -27.3 }, { 106, -29.9 }, { 108, -30.5 } } };

double noteHz(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

std::vector<double> renderString(ModalGrandString & string, double seconds, double dampAt = -1.0)
{
    std::vector<double> out;
    for (size_t i = 0; i < static_cast<size_t>(seconds * SampleRate); i++) {
        if (dampAt >= 0.0 && i == static_cast<size_t>(dampAt * SampleRate)) {
            string.damp();
        }
        double l = 0.0;
        double r = 0.0;
        string.process(l, r);
        out.push_back(0.5 * (l + r));
    }
    return out;
}

//! The strongest frequency near @p nominalHz, from a long Hann-windowed DFT around it
double peakHz(const std::vector<double> & x, double nominalHz)
{
    const double span = nominalHz * 0.02;
    double bestHz = nominalHz;
    double best = 0.0;
    const auto n = x.size();
    for (double hz = nominalHz - span; hz <= nominalHz + span; hz += nominalHz * 1.0e-5) {
        std::complex<double> sum {};
        const double w = 2.0 * M_PI * hz / SampleRate;
        for (size_t i = 0; i < n; i += 1) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(n));
            sum += x.at(i) * window * std::polar(1.0, -w * static_cast<double>(i));
        }
        if (std::abs(sum) > best) {
            best = std::abs(sum);
            bestHz = hz;
        }
    }
    return bestHz;
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

double db(double value)
{
    return 20.0 * std::log10(std::max(value, 1.0e-12));
}

//! Renders @p seconds of the device, as the louder of its two channels
void renderDevice(PianoSynthAcousticDevice & device, double seconds, std::vector<double> & out)
{
    const auto frames = static_cast<uint32_t>(seconds * SampleRate);
    std::vector<double> block(512 * 2);
    for (uint32_t done = 0; done < frames; done += 512) {
        std::ranges::fill(block, 0.0);
        AudioContext context { .buffer = block, .frameCount = 512, .sampleRate = static_cast<uint32_t>(SampleRate) };
        device.processAudio(context);
        for (size_t i = 0; i < 512; i++) {
            out.push_back(std::max(std::abs(block.at(i * 2)), std::abs(block.at(i * 2 + 1))));
        }
    }
}

double notePeakDb(PianoSynthAcousticDevice & device, int note, int velocity = 100)
{
    device.processMidiNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity));
    std::vector<double> x;
    renderDevice(device, 0.4, x);
    device.processMidiNoteOff(static_cast<uint8_t>(note));
    renderDevice(device, 0.5, x);
    return db(peak(x));
}

//! How much louder the right channel is than the left, in dB, over the first half second of @p note
double rightOverLeftDb(float panSpread, int note)
{
    PianoSynthAcousticDevice device { "Piano" };
    device.setKnob(Xml::xmlKeyPanSpread(), panSpread);
    device.processMidiNoteOn(static_cast<uint8_t>(note), 100);
    double left = 0.0;
    double right = 0.0;
    std::vector<double> block(512 * 2);
    for (int b = 0; b < 48; b++) {
        std::ranges::fill(block, 0.0);
        AudioContext context { .buffer = block, .frameCount = 512, .sampleRate = static_cast<uint32_t>(SampleRate) };
        device.processAudio(context);
        for (size_t i = 0; i < 512; i++) {
            left += block.at(i * 2) * block.at(i * 2);
            right += block.at(i * 2 + 1) * block.at(i * 2 + 1);
        }
    }
    return 10.0 * std::log10(right / left);
}

ModalGrandString::Settings noUnison()
{
    ModalGrandString::Settings settings;
    settings.detune = 0.0f;
    return settings;
}

} // namespace

void PianoSynthAcousticTest::test_string_strike_shouldSoundAtTheStretchedPitch()
{
    ModalGrandString string;
    string.setSampleRate(SampleRate);
    string.strike(96, 0.8f, noUnison(), false);
    const auto x = renderString(string, 0.5);
    const double expected = noteHz(96) * std::exp2(ModalGrandString::stretchCents(96) / 1200.0);
    QVERIFY(ModalGrandString::stretchCents(96) > 10.0);
    QVERIFY(std::abs(1200.0 * std::log2(peakHz(x, expected) / expected)) < 1.0);
}

void PianoSynthAcousticTest::test_string_strike_stretchOff_shouldSoundEqualTempered()
{
    ModalGrandString string;
    string.setSampleRate(SampleRate);
    auto settings = noUnison();
    settings.stretch = 0.0f;
    string.strike(96, 0.8f, settings, false);
    const auto x = renderString(string, 0.5);
    QVERIFY(std::abs(1200.0 * std::log2(peakHz(x, noteHz(96)) / noteHz(96))) < 1.0);
}

void PianoSynthAcousticTest::test_string_damp_shouldDieFasterThanHeld()
{
    ModalGrandString held;
    held.setSampleRate(SampleRate);
    held.strike(60, 0.8f, {}, false);
    ModalGrandString damped;
    damped.setSampleRate(SampleRate);
    damped.strike(60, 0.8f, {}, false);
    const auto a = renderString(held, 0.6);
    const auto b = renderString(damped, 0.6, 0.1);
    QVERIFY(db(rms(b, 0.5, 0.6)) < db(rms(a, 0.5, 0.6)) - 30.0);
}

void PianoSynthAcousticTest::test_string_damp_undampedNote_shouldKeepRinging()
{
    ModalGrandString held;
    held.setSampleRate(SampleRate);
    held.strike(100, 0.8f, {}, false);
    ModalGrandString damped;
    damped.setSampleRate(SampleRate);
    damped.strike(100, 0.8f, {}, false);
    const auto a = renderString(held, 0.5);
    const auto b = renderString(damped, 0.5, 0.1);
    QVERIFY(std::abs(db(rms(b, 0.4, 0.5)) - db(rms(a, 0.4, 0.5))) < 0.1);
}

void PianoSynthAcousticTest::test_device_sustainPedal_shouldHoldReleasedNotes()
{
    const auto level = [](bool pedal) {
        PianoSynthAcousticDevice device { "Piano" };
        device.processDeviceMidiCc(64, pedal ? 127 : 0, 0);
        device.processMidiNoteOn(60, 100);
        std::vector<double> x;
        renderDevice(device, 0.1, x);
        device.processMidiNoteOff(60);
        renderDevice(device, 0.5, x);
        return db(rms(x, 0.5, 0.6));
    };
    QVERIFY(level(true) > level(false) + 30.0);
}

void PianoSynthAcousticTest::test_device_resonance_pedalDown_shouldRingTheOtherStrings()
{
    const auto level = [](float resonance) {
        PianoSynthAcousticDevice device { "Piano" };
        device.setKnob(Xml::xmlKeyResonance(), resonance);
        device.processDeviceMidiCc(64, 127, 0);
        device.processMidiNoteOn(48, 100);
        std::vector<double> x;
        renderDevice(device, 1.5, x);
        return db(rms(x, 1.0, 1.5));
    };
    QVERIFY(level(1.0f) > level(0.0f) + 0.5);
}

void PianoSynthAcousticTest::test_device_softPedal_shouldSoftenTheStrike()
{
    PianoSynthAcousticDevice normal { "Piano" };
    PianoSynthAcousticDevice soft { "Piano" };
    soft.processDeviceMidiCc(67, 127, 0);
    QVERIFY(notePeakDb(soft, 60) < notePeakDb(normal, 60) - 2.0);
}

void PianoSynthAcousticTest::test_device_panSpread_shouldPutTheBassLeftAndTheTrebleRight()
{
    QVERIFY(rightOverLeftDb(0.6f, 28) < -6.0);
    QVERIFY(rightOverLeftDb(0.6f, 96) > 6.0);
    QVERIFY(std::abs(rightOverLeftDb(0.6f, 60)) < 1.5);
}

void PianoSynthAcousticTest::test_device_panSpread_zero_shouldCentreEveryKey()
{
    QVERIFY(std::abs(rightOverLeftDb(0.0f, 28)) < 0.5);
    QVERIFY(std::abs(rightOverLeftDb(0.0f, 96)) < 0.5);
}

void PianoSynthAcousticTest::test_device_restrike_shouldKeepOneVoicePerStrike()
{
    PianoSynthAcousticDevice device { "Piano" };
    device.processMidiNoteOn(60, 100);
    std::vector<double> x;
    renderDevice(device, 0.05, x);
    device.processMidiNoteOn(60, 100);
    renderDevice(device, 0.05, x);
    // The first string rings out under its damper while the second sounds
    QCOMPARE(device.activeVoiceCount(), 2);
}

void PianoSynthAcousticTest::test_keyboard_peaks_shouldFollowTheRecording()
{
    double referenceMean = 0.0;
    double ownMean = 0.0;
    std::vector<double> own;
    for (auto && [note, reference] : ReferencePeaks) {
        PianoSynthAcousticDevice device { "Piano" };
        own.push_back(notePeakDb(device, note));
        referenceMean += reference / static_cast<double>(ReferencePeaks.size());
        ownMean += own.back() / static_cast<double>(ReferencePeaks.size());
    }
    for (size_t i = 0; i < ReferencePeaks.size(); i++) {
        const double error = (own.at(i) - ownMean) - (ReferencePeaks.at(i).second - referenceMean);
        QVERIFY2(std::abs(error) < 3.0, std::to_string(ReferencePeaks.at(i).first).c_str());
    }
}

void PianoSynthAcousticTest::test_presets_every_shouldPeakNearInit()
{
    double initPeak = 0.0;
    for (size_t i = 0; i < PianoSynthAcousticPresets::presets().size(); i++) {
        PianoSynthAcousticDevice device { "Piano" };
        device.loadPreset(static_cast<int>(i));
        const double level = notePeakDb(device, 60);
        if (i == 0) {
            initPeak = level;
        }
        QVERIFY2(std::abs(level - initPeak) < 1.5, PianoSynthAcousticPresets::presets().at(i).name.c_str());
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PianoSynthAcousticTest)
