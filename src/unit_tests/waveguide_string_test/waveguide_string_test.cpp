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

#include "waveguide_string_test.hpp"

#include "../../domain/dsp/waveguide_string.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

namespace noteahead {

namespace {

double noteFrequency(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

double centsBetween(double measured, double reference)
{
    return 1200.0 * std::log2(measured / reference);
}

// Discards the strike transient and returns the steady part of the tone.
std::vector<double> renderString(WaveguideString & string, size_t skip, size_t length)
{
    for (size_t i = 0; i < skip; i++) {
        string.nextSample();
    }
    std::vector<double> samples(length);
    for (size_t i = 0; i < length; i++) {
        samples[i] = string.nextSample();
    }
    return samples;
}

std::vector<double> hannWindow(size_t length)
{
    std::vector<double> window(length);
    for (size_t i = 0; i < length; i++) {
        window[i] = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(length - 1));
    }
    return window;
}

// Magnitude of the windowed DFT at an arbitrary frequency, evaluated by rotating a
// phasor so that the frequency grid can be made as fine as the test needs.
double dftMagnitude(const std::vector<double> & samples, const std::vector<double> & window, double sampleRate, double frequency)
{
    const double w = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double cosW = std::cos(w);
    const double sinW = std::sin(w);
    double phasorRe = 1.0;
    double phasorIm = 0.0;
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        const double value = samples[i] * window[i];
        re += value * phasorRe;
        im -= value * phasorIm;
        const double nextRe = phasorRe * cosW - phasorIm * sinW;
        phasorIm = phasorRe * sinW + phasorIm * cosW;
        phasorRe = nextRe;
    }
    return std::hypot(re, im);
}

// Locates the fundamental by scanning a fine grid spanning ±4 % around the expected
// frequency and refining the winning bin parabolically. Good to well under a cent.
// Nothing is returned if the peak sits at the edge of the span, since that means the
// real fundamental is somewhere further out and was never actually measured.
std::optional<double> measureFundamental(const std::vector<double> & samples, double sampleRate, double expected)
{
    const auto window = hannWindow(samples.size());
    constexpr int steps = 80;
    const double step = expected * 0.0005;

    std::vector<double> magnitudes(2 * steps + 1);
    int best = 0;
    for (int i = 0; i <= 2 * steps; i++) {
        magnitudes[i] = dftMagnitude(samples, window, sampleRate, expected + (i - steps) * step);
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

// Longer windows for low notes so that the fundamental stays resolvable, shorter ones
// for high notes so that the scan stays cheap.
size_t windowLength(double sampleRate, double frequency)
{
    return std::clamp(static_cast<size_t>(40.0 * sampleRate / frequency), size_t { 8192 }, size_t { 32768 });
}

std::optional<double> measureStringPitch(double sampleRate, int note, float brightness = 0.5f, float inharmonicity = 0.0f, double detuneCents = 0.0, float velocity = 0.8f)
{
    WaveguideString string;
    string.setSampleRate(sampleRate);
    string.trigger(static_cast<uint8_t>(note), velocity, brightness, inharmonicity, 1.0f, detuneCents);

    const double expected = noteFrequency(note) * std::exp2(detuneCents / 1200.0);
    const auto samples = renderString(string, 4096, windowLength(sampleRate, expected));
    return measureFundamental(samples, sampleRate, expected);
}

void verifyPitch(std::optional<double> measured, double expected, double toleranceCents, const QString & context)
{
    QVERIFY2(measured.has_value(),
             QString { "%1: no fundamental found within 4 %% of %2 Hz" }.arg(context).arg(expected).toUtf8().constData());

    const double cents = centsBetween(measured.value(), expected);
    QVERIFY2(std::abs(cents) < toleranceCents,
             QString { "%1: expected %2 Hz, measured %3 Hz, off by %4 cents" }
               .arg(context)
               .arg(expected)
               .arg(measured.value())
               .arg(cents)
               .toUtf8()
               .constData());
}

} // namespace

void WaveguideStringTest::test_tuning_shouldMatchNoteFrequency_acrossMidiRange()
{
    constexpr double sampleRate = 44100.0;

    // The top octaves are where a whole-sample loop length used to be tens of cents off.
    for (const int note : { 21, 33, 45, 57, 60, 69, 79, 88, 96 }) {
        const double expected = noteFrequency(note);
        verifyPitch(measureStringPitch(sampleRate, note), expected, 2.0, QString { "Note %1" }.arg(note));
    }
}

void WaveguideStringTest::test_tuning_shouldMatchNoteFrequency_atHighSampleRate()
{
    // At 96 kHz the loop for the bottom notes is longer than the delay line used to be,
    // which left them clamped to a much shorter and therefore far sharper period.
    for (const double sampleRate : { 48000.0, 96000.0 }) {
        for (const int note : { 21, 28, 60, 88 }) {
            const double expected = noteFrequency(note);
            verifyPitch(measureStringPitch(sampleRate, note), expected, 2.0,
                        QString { "Note %1 at %2 Hz" }.arg(note).arg(sampleRate));
        }
    }
}

void WaveguideStringTest::test_tuning_shouldMatchNoteFrequency_whenBrightnessVaries()
{
    constexpr double sampleRate = 44100.0;
    constexpr int note = 79;
    const double expected = noteFrequency(note);

    // Brightness sets the loop filter coefficient, which adds delay of its own. Playing
    // the same note harder or softer must not move its pitch.
    for (const float brightness : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }) {
        verifyPitch(measureStringPitch(sampleRate, note, brightness), expected, 2.0,
                    QString { "Brightness %1" }.arg(brightness));
    }
}

void WaveguideStringTest::test_tuning_shouldMatchNoteFrequency_whenInharmonicityApplied()
{
    constexpr double sampleRate = 44100.0;

    // Dispersion stretches the partials, but the fundamental must stay on the note.
    for (const int note : { 45, 69, 88 }) {
        const double expected = noteFrequency(note);
        for (const float inharmonicity : { 0.0f, 0.5f, 1.0f }) {
            verifyPitch(measureStringPitch(sampleRate, note, 0.5f, inharmonicity), expected, 3.0,
                        QString { "Note %1, inharmonicity %2" }.arg(note).arg(inharmonicity));
        }
    }
}

void WaveguideStringTest::test_tuning_shouldFollowDetune_atSubSemitoneResolution()
{
    constexpr double sampleRate = 44100.0;
    constexpr int note = 60;
    const auto reference = measureStringPitch(sampleRate, note);
    QVERIFY(reference.has_value());

    // A whole-sample loop length quantised small detunings away entirely, which left the
    // unison pairs of the piano perfectly in tune with each other and thus beat-free.
    for (const double detuneCents : { 1.5, 3.0, 7.5 }) {
        const auto measured = measureStringPitch(sampleRate, note, 0.5f, 0.0f, detuneCents);
        QVERIFY(measured.has_value());
        const double applied = centsBetween(measured.value(), reference.value());
        QVERIFY2(std::abs(applied - detuneCents) < 0.5,
                 QString { "Detune of %1 cents came out as %2 cents" }.arg(detuneCents).arg(applied).toUtf8().constData());
    }
}

void WaveguideStringTest::test_tuning_shouldRecover_whenSampleRateChangesAfterTrigger()
{
    constexpr int note = 57;
    constexpr double sampleRate = 44100.0;

    // Notes struck before the audio backend reports its rate are tuned against the
    // default one, so the string has to re-derive its loop length when told otherwise.
    WaveguideString string;
    string.setSampleRate(48000.0);
    string.trigger(note, 0.8f, 0.5f, 0.0f, 1.0f);
    string.setSampleRate(sampleRate);

    const double expected = noteFrequency(note);
    const auto samples = renderString(string, 4096, windowLength(sampleRate, expected));
    verifyPitch(measureFundamental(samples, sampleRate, expected), expected, 2.0, "Retuned string");
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WaveguideStringTest)
