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

#include "piano_synth_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/piano_synth_device.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

namespace noteahead {

namespace {

AudioContext makeContext(std::vector<double> & buffer, uint32_t frameCount, uint32_t sampleRate = 44100)
{
    return AudioContext { std::span(buffer.data(), buffer.size()), frameCount, sampleRate };
}

double peakLevel(const std::vector<double> & buffer)
{
    double peak = 0.0;
    for (const double s : buffer) {
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

double rmsLevel(const std::vector<double> & buffer)
{
    double sum = 0.0;
    for (const double s : buffer) {
        sum += s * s;
    }
    return std::sqrt(sum / static_cast<double>(buffer.size()));
}

void renderFrames(PianoSynthDevice & piano, uint32_t frames, uint32_t sampleRate = 44100)
{
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    auto ctx = makeContext(buffer, frames, sampleRate);
    piano.processAudio(ctx);
}

// Renders the device down to mono, discarding the strike transient so that only the
// steady part of the tone is left to measure.
std::vector<double> renderTone(PianoSynthDevice & piano, uint32_t sampleRate, uint32_t skip, uint32_t length)
{
    const uint32_t frames = skip + length;
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    auto ctx = makeContext(buffer, frames, sampleRate);
    piano.processAudio(ctx);

    std::vector<double> mono(length);
    for (uint32_t i = 0; i < length; i++) {
        const size_t frame = static_cast<size_t>(skip + i) * 2;
        mono[i] = buffer[frame] + buffer[frame + 1];
    }
    return mono;
}

// Magnitude of the Hann-windowed DFT at an arbitrary frequency, evaluated by rotating
// a phasor so that the frequency grid can be made as fine as needed.
double dftMagnitude(const std::vector<double> & samples, double sampleRate, double frequency)
{
    const double w = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double cosW = std::cos(w);
    const double sinW = std::sin(w);
    const double last = static_cast<double>(samples.size() - 1);
    double phasorRe = 1.0;
    double phasorIm = 0.0;
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / last);
        const double value = samples[i] * window;
        re += value * phasorRe;
        im -= value * phasorIm;
        const double nextRe = phasorRe * cosW - phasorIm * sinW;
        phasorIm = phasorRe * sinW + phasorIm * cosW;
        phasorRe = nextRe;
    }
    return std::hypot(re, im);
}

// Locates the fundamental on a fine grid spanning ±3 % of the expected frequency and
// refines the winning bin parabolically. Nothing is returned if the peak sits at the
// edge of the span, since the real fundamental is then somewhere further out.
std::optional<double> measureFundamental(const std::vector<double> & samples, double sampleRate, double expected)
{
    constexpr int steps = 60;
    const double step = expected * 0.0005;

    std::vector<double> magnitudes(2 * steps + 1);
    int best = 0;
    for (int i = 0; i <= 2 * steps; i++) {
        magnitudes[i] = dftMagnitude(samples, sampleRate, expected + (i - steps) * step);
        if (magnitudes[i] > magnitudes[best]) {
            best = i;
        }
    }
    if (best == 0 || best == 2 * steps) {
        return std::nullopt;
    }

    const double denominator = magnitudes[best - 1] - 2.0 * magnitudes[best] + magnitudes[best + 1];
    const double offset = denominator == 0.0 ? 0.0 : 0.5 * (magnitudes[best - 1] - magnitudes[best + 1]) / denominator;
    return expected + (best - steps + offset) * step;
}

double noteFrequency(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

double centsBetween(double measured, double reference)
{
    return 1200.0 * std::log2(measured / reference);
}

// Plays one note on an otherwise clean voice and returns its sounding frequency.
std::optional<double> measurePitch(int note, uint8_t velocity = 100, uint32_t sampleRate = 44100)
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setVolume(1.0f);
    piano.setGain(0.5f);
    piano.setDecay(1.0f);
    piano.setStringDetune(0.0f); // Keep the unison pair together so there is one pitch
    piano.processMidiNoteOn(static_cast<uint8_t>(note), velocity);

    const double expected = noteFrequency(note);
    const auto length = static_cast<uint32_t>(std::clamp(40.0 * sampleRate / expected, 8192.0, 32768.0));
    const auto mono = renderTone(piano, sampleRate, 4096, length);
    return measureFundamental(mono, sampleRate, expected);
}

} // namespace

void PianoSynthTest::test_midiNoteOn_shouldActivateAudio()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.processMidiNoteOn(60, 100);
    QVERIFY(piano.hasActiveAudio());
}

void PianoSynthTest::test_midiNoteOff_shouldDecayToSilence()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setReleaseTime(0.0f); // Shortest possible damper
    piano.processMidiNoteOn(60, 100);

    // Let the string speak briefly
    renderFrames(piano, 512);
    QVERIFY(piano.hasActiveAudio());

    piano.processMidiNoteOff(60);

    // Render enough for the fast damper to quiet the string
    renderFrames(piano, 44100);
    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthTest::test_polyphony_shouldSupportMultipleSimultaneousNotes()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.processMidiNoteOn(60, 100);
    piano.processMidiNoteOn(64, 100);
    piano.processMidiNoteOn(67, 100);

    // Compare energy rather than peak level: the hammer strikes of a chord are spread
    // over a couple of milliseconds on purpose, so three notes carry three times the
    // sound without stacking into a taller transient.
    const uint32_t frameCount = 8192;
    const double rmsThree = rmsLevel(renderTone(piano, 44100, 0, frameCount));

    PianoSynthDevice pianoOne { "Test Piano One" };
    pianoOne.processMidiNoteOn(60, 100);
    const double rmsOne = rmsLevel(renderTone(pianoOne, 44100, 0, frameCount));

    QVERIFY2(rmsThree > rmsOne,
             QString { "Three-note level (%1) not greater than one-note level (%2)" }.arg(rmsThree).arg(rmsOne).toUtf8().constData());
}

void PianoSynthTest::test_sustainPedal_shouldKeepNoteActiveAfterNoteOff()
{
    PianoSynthDevice piano { "Test Piano" };

    // Pedal down (CC 64 >= 64)
    piano.processMidiCc(64, 127, 0);
    piano.processMidiNoteOn(60, 100);
    renderFrames(piano, 256);

    piano.processMidiNoteOff(60);

    // With pedal held, the voice must still be active immediately after note-off
    QVERIFY(piano.hasActiveAudio());
}

void PianoSynthTest::test_sustainPedal_shouldReleaseNoteWhenPedalLifted()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setReleaseTime(0.0f);

    piano.processMidiCc(64, 127, 0); // pedal down
    piano.processMidiNoteOn(60, 100);
    renderFrames(piano, 256);
    piano.processMidiNoteOff(60);

    // Lift pedal
    piano.processMidiCc(64, 0, 0);

    // Render enough for the damper to silence the string
    renderFrames(piano, 44100);
    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthTest::test_allNotesOff_shouldSilenceAllVoices()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setReleaseTime(0.0f);

    piano.processMidiNoteOn(48, 100);
    piano.processMidiNoteOn(60, 100);
    piano.processMidiNoteOn(72, 100);
    renderFrames(piano, 256);

    piano.processMidiAllNotesOff();
    renderFrames(piano, 44100);

    QVERIFY(!piano.hasActiveAudio());
}

void PianoSynthTest::test_serialization_shouldRestoreParameters()
{
    PianoSynthDevice piano { "Test Piano" };
    piano.setBrightness(0.8f);
    piano.setDecay(0.3f);
    piano.setInharmonicity(0.1f);

    QString xml;
    NahdXmlWriter writer { xml };
    piano.serializeToXml(writer);

    PianoSynthDevice piano2 { "Restored Piano" };
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        piano2.deserializeFromXml(reader);
    }

    QCOMPARE(piano2.brightness(), 0.8f);
    QCOMPARE(piano2.decay(), 0.3f);
    QCOMPARE(piano2.inharmonicity(), 0.1f);
}

void PianoSynthTest::test_velocity_shouldAffectOutputLevel()
{
    auto getPeak = [](uint8_t velocity) {
        PianoSynthDevice piano { "Test Piano" };
        piano.setVolume(1.0f);
        piano.setGain(0.5f);
        piano.processMidiNoteOn(60, velocity);

        const uint32_t frameCount = 512;
        std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
        auto ctx = makeContext(buffer, frameCount);
        piano.processAudio(ctx);
        return peakLevel(buffer);
    };

    const double peakLow = getPeak(30);
    const double peakHigh = getPeak(127);

    QVERIFY2(peakHigh > peakLow,
             QString("Velocity did not affect level: low=%1 high=%2").arg(peakLow).arg(peakHigh).toUtf8().constData());
}

void PianoSynthTest::test_tuning_shouldMatchNoteFrequency_acrossKeyboard()
{
    for (const int note : { 21, 36, 48, 60, 69, 79, 88, 96 }) {
        const double expected = noteFrequency(note);
        const auto measured = measurePitch(note);
        QVERIFY2(measured.has_value(),
                 QString { "Note %1: no fundamental found near %2 Hz" }.arg(note).arg(expected).toUtf8().constData());

        const double cents = centsBetween(measured.value(), expected);
        QVERIFY2(std::abs(cents) < 3.0,
                 QString { "Note %1: expected %2 Hz, measured %3 cents off" }
                   .arg(note)
                   .arg(expected)
                   .arg(cents)
                   .toUtf8()
                   .constData());
    }
}

void PianoSynthTest::test_tuning_shouldNotDependOnVelocity()
{
    constexpr int note = 79;

    // Velocity feeds the hammer brightness, which sets the loop filter and hence part of
    // the loop delay. How hard the key is struck must not change what note comes out.
    const auto soft = measurePitch(note, 20);
    const auto hard = measurePitch(note, 127);
    QVERIFY(soft.has_value());
    QVERIFY(hard.has_value());

    const double difference = centsBetween(hard.value(), soft.value());

    QVERIFY2(std::abs(difference) < 1.0,
             QString { "Velocity shifted the pitch by %1 cents" }.arg(difference).toUtf8().constData());
}

void PianoSynthTest::test_chord_shouldNotStackHammerStrikes()
{
    constexpr uint32_t sampleRate = 44100;
    constexpr uint32_t attackFrames = 512; // ~12 ms, the whole strike
    const std::vector<int> chord = { 56, 60, 63 };

    // The hammer pulse is one-sided, so when every voice loaded it at the same point in
    // its delay line the notes of a chord added up into a spike far above any of them.
    double loudestAlone = 0.0;
    for (const int note : chord) {
        PianoSynthDevice single { "Test Piano" };
        single.processMidiNoteOn(static_cast<uint8_t>(note), 127);
        loudestAlone = std::max(loudestAlone, peakLevel(renderTone(single, sampleRate, 0, attackFrames)));
    }

    PianoSynthDevice piano { "Test Piano" };
    for (const int note : chord) {
        piano.processMidiNoteOn(static_cast<uint8_t>(note), 127);
    }
    const double together = peakLevel(renderTone(piano, sampleRate, 0, attackFrames));

    // Struck together they used to reach nearly the sum of all three
    QVERIFY2(together < loudestAlone * 1.6,
             QString { "Chord attack %1 against %2 for the loudest note alone" }
               .arg(together)
               .arg(loudestAlone)
               .toUtf8()
               .constData());
}

void PianoSynthTest::test_brightness_shouldStayEffective_whenStruckHard()
{
    constexpr uint32_t sampleRate = 44100;

    // Velocity and register used to be added on top of Brightness and the sum clamped, so
    // above the clamp the control did nothing whatsoever: these two settings rendered
    // sample for sample identically on a hard-struck note.
    const auto render = [](float brightness) {
        PianoSynthDevice piano { "Test Piano" };
        piano.setBrightness(brightness);
        piano.processMidiNoteOn(72, 127);
        return renderTone(piano, sampleRate, 0, 8192);
    };

    const auto dark = render(0.5f);
    const auto bright = render(0.9f);

    double difference = 0.0;
    for (size_t i = 0; i < dark.size(); i++) {
        difference = std::max(difference, std::abs(dark[i] - bright[i]));
    }

    QVERIFY2(difference > 1e-4,
             QString { "Brightness 0.5 and 0.9 differ by only %1 at velocity 127" }.arg(difference).toUtf8().constData());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PianoSynthTest)
