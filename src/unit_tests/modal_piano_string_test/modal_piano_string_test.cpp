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

#include "modal_piano_string_test.hpp"

#include "../../domain/dsp/modal_piano_string.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

double noteFrequency(int note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

std::vector<double> render(ModalPianoString & string, size_t samples)
{
    std::vector<double> out(samples);
    for (size_t i = 0; i < samples; i++) {
        out[i] = string.nextSample();
    }
    return out;
}

double peakLevel(std::span<const double> samples)
{
    double peak = 0.0;
    for (const double s : samples) {
        peak = std::max(peak, std::abs(s));
    }
    return peak;
}

// Magnitude of the Hann-windowed DFT at an arbitrary frequency, so that the frequency
// grid can be made as fine as the pitch check needs.
double dftMagnitude(std::span<const double> samples, double sampleRate, double frequency)
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

std::optional<double> measurePitch(std::span<const double> samples, double expected)
{
    constexpr int steps = 60;
    const double step = expected * 0.0005;

    std::vector<double> magnitudes(2 * steps + 1);
    int best = 0;
    for (int i = 0; i <= 2 * steps; i++) {
        magnitudes[i] = dftMagnitude(samples, SampleRate, expected + (i - steps) * step);
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

double centsBetween(double measured, double reference)
{
    return 1200.0 * std::log2(measured / reference);
}

ModalPianoString::Settings settings()
{
    // The unison pair is kept together throughout, so that a measured pitch is one pitch
    // and an envelope does not wander with the beating.
    ModalPianoString::Settings s;
    s.detune = 0.0f;
    return s;
}

} // namespace

void ModalPianoStringTest::test_trigger_shouldSound()
{
    ModalPianoString string;
    string.setSampleRate(SampleRate);
    QVERIFY(!string.isActive());

    string.trigger(60, 1.0f, settings());
    QVERIFY(string.isActive());
    QVERIFY(peakLevel(render(string, 4800)) > 1e-3);
}

void ModalPianoStringTest::test_trigger_shouldOpenFromSilence()
{
    // Every mode starts on its own rising quarter cycle and the strike is ramped, so the
    // note must not step straight to its level on the first sample.
    ModalPianoString string;
    string.setSampleRate(SampleRate);
    string.trigger(60, 1.0f, settings());

    const auto out = render(string, 4800);
    const double peak = peakLevel(out);
    QVERIFY(peak > 0.0);
    QVERIFY2(std::abs(out.front()) < peak * 0.01,
             QString { "First sample was %1 against a peak of %2" }.arg(out.front()).arg(peak).toUtf8().constData());
}

void ModalPianoStringTest::test_trigger_shouldPlaceFundamentalAtNotePitch()
{
    // Stiffness pushes every partial sharp, the first one included. The bank is brought
    // back down by what it does to the first partial, so the note sounds where it is asked
    // to — which matters most at the top, where the coefficient is largest.
    for (const int note : { 21, 48, 72, 96, 108 }) {
        ModalPianoString string;
        string.setSampleRate(SampleRate);
        string.trigger(static_cast<uint8_t>(note), 1.0f, settings());

        const auto out = render(string, 24000);
        const auto measured = measurePitch(out, noteFrequency(note));
        QVERIFY2(measured.has_value(), QString { "No pitch found for note %1" }.arg(note).toUtf8().constData());

        const double cents = centsBetween(*measured, noteFrequency(note));
        QVERIFY2(std::abs(cents) < 3.0,
                 QString { "Note %1 sounded %2 cents off" }.arg(note).arg(cents).toUtf8().constData());
    }
}

void ModalPianoStringTest::test_richness_shouldSetPartialCount()
{
    auto s = settings();

    ModalPianoString lean;
    lean.setSampleRate(SampleRate);
    s.richness = 0.0f;
    lean.trigger(36, 1.0f, s);

    ModalPianoString full;
    full.setSampleRate(SampleRate);
    s.richness = 1.0f;
    full.trigger(36, 1.0f, s);

    QVERIFY2(full.activeModeCount() > lean.activeModeCount(),
             QString { "Full bank has %1 modes against the lean one's %2" }.arg(full.activeModeCount()).arg(lean.activeModeCount()).toUtf8().constData());
}

void ModalPianoStringTest::test_richness_shouldStayBelowNyquist()
{
    // A top note asked for every partial it can have must stop short of Nyquist rather
    // than folding the rest back into the note.
    auto s = settings();
    s.richness = 1.0f;

    ModalPianoString string;
    string.setSampleRate(SampleRate);
    string.trigger(108, 1.0f, s);

    // Only a handful of partials fit under Nyquist above four kilohertz.
    QVERIFY2(string.activeModeCount() > 0 && string.activeModeCount() < 20,
             QString { "Top note built %1 modes" }.arg(string.activeModeCount()).toUtf8().constData());
}

void ModalPianoStringTest::test_release_shouldSilenceTheString()
{
    ModalPianoString string;
    string.setSampleRate(SampleRate);
    string.trigger(60, 1.0f, settings());
    render(string, 480);

    string.release(0.0f);
    render(string, static_cast<size_t>(SampleRate));

    QVERIFY(!string.isActive());
}

void ModalPianoStringTest::test_release_shouldBeIgnored_forUndampedNotes()
{
    // A grand has no dampers over the top octave and a half.
    ModalPianoString string;
    string.setSampleRate(SampleRate);
    string.trigger(ModalPianoString::LowestUndampedNote, 1.0f, settings());
    render(string, 480);

    string.release(0.0f);
    QVERIFY(peakLevel(render(string, 4800)) > 1e-4);
    QVERIFY(string.isActive());
}

void ModalPianoStringTest::test_reset_shouldSilenceTheString()
{
    ModalPianoString string;
    string.setSampleRate(SampleRate);
    string.trigger(60, 1.0f, settings());
    render(string, 480);

    string.reset();

    QVERIFY(!string.isActive());
    QCOMPARE(string.activeModeCount(), 0);
    QCOMPARE(peakLevel(render(string, 480)), 0.0);
}

void ModalPianoStringTest::test_sampleRate_shouldHoldPitch_whenChangedMidNote()
{
    // A note struck before the backend has reported its rate was given poles derived
    // against the old one. Carrying them across must leave the note where it was rather
    // than transposing it.
    ModalPianoString string;
    string.setSampleRate(44100.0);
    string.trigger(60, 1.0f, settings());
    render(string, 512);

    string.setSampleRate(SampleRate);
    const auto out = render(string, 24000);

    const auto measured = measurePitch(out, noteFrequency(60));
    QVERIFY(measured.has_value());
    const double cents = centsBetween(*measured, noteFrequency(60));
    QVERIFY2(std::abs(cents) < 3.0,
             QString { "Note moved %1 cents across the rate change" }.arg(cents).toUtf8().constData());
}

void ModalPianoStringTest::test_modes_shouldBePruned_asTheNoteDecays()
{
    // The partials at the top go first, and once they have gone they must stop costing
    // anything. This is what keeps a held chord affordable.
    ModalPianoString string;
    string.setSampleRate(SampleRate);
    string.trigger(84, 1.0f, settings());

    const int struck = string.activeModeCount();
    render(string, static_cast<size_t>(SampleRate * 4.0));

    QVERIFY2(string.activeModeCount() < struck,
             QString { "Still computing all %1 modes four seconds in" }.arg(struck).toUtf8().constData());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ModalPianoStringTest)
