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

#include "poly_blep_oscillator_test.hpp"
#include "../../domain/dsp/poly_blep_oscillator.hpp"

#include <QTest>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

//! Energy sitting on anything that is not a harmonic of the note. Above Nyquist a harmonic either
//! was filtered away or folded back to some unrelated frequency, and it is that folded rubbish this
//! measures. The frequency is chosen so a period is not a whole number of samples: otherwise a
//! folded harmonic lands on another harmonic's bin and is counted as signal.
double aliasingRatio(PolyBlepOscillator::Waveform waveform, double frequency, double sampleRate, double shape)
{
    constexpr int periods = 97;

    PolyBlepOscillator osc;
    osc.setSampleRate(sampleRate);
    osc.setFrequency(frequency);
    osc.setWaveform(waveform);
    osc.setShape(shape);

    const auto total = static_cast<size_t>(std::round(static_cast<double>(periods) * sampleRate / frequency));
    for (size_t i = 0; i < total; i++) {
        osc.nextSample(); // Warm-up: the pulse stage's coupling settles from silence
    }
    std::vector<double> samples;
    samples.reserve(total);
    for (size_t i = 0; i < total; i++) {
        samples.push_back(osc.nextSample());
    }

    const auto n = static_cast<double>(samples.size());
    const auto magnitude = [&](int harmonic) {
        const double bin = static_cast<double>(harmonic * periods);
        double re = 0.0;
        double im = 0.0;
        for (size_t i = 0; i < samples.size(); i++) {
            const double angle = 2.0 * std::numbers::pi * bin * static_cast<double>(i) / n;
            re += samples[i] * std::cos(angle);
            im += samples[i] * std::sin(angle);
        }
        return 2.0 * std::hypot(re, im) / n;
    };

    double total2 = 0.0;
    for (const double sample : samples) {
        total2 += sample * sample;
    }
    double harmonics = 0.0;
    for (int harmonic = 1; static_cast<double>(harmonic * periods) < n * 0.5; harmonic++) {
        const double m = magnitude(harmonic);
        harmonics += m * m * 0.5 * n;
    }
    return total2 > 0.0 ? std::max(0.0, total2 - harmonics) / total2 : 0.0;
}

} // namespace

void PolyBlepOscillatorTest::test_shapedSaw_shouldNotAliasMoreThanThePlainOne()
{
    // Shaping bends both ends of the ramp towards zero, so the step left at the wrap is smaller
    // than the one the correction is sized for. Correcting the full step on a shaped wave
    // over-corrects, and the over-correction folds back into the band: at 880 Hz with the shaper
    // most of the way up this measured sixteen times the aliasing of the plain saw.
    const double plain = aliasingRatio(PolyBlepOscillator::Waveform::Saw, 880.0, 48000.0, 0.0);
    const double shaped = aliasingRatio(PolyBlepOscillator::Waveform::Saw, 880.0, 48000.0, 0.9);

    QVERIFY(shaped <= plain);

    // Fully shaped the wave is a sine, which has no discontinuity to correct at all
    QVERIFY(aliasingRatio(PolyBlepOscillator::Waveform::Saw, 880.0, 48000.0, 1.0) < plain * 0.1);
}

void PolyBlepOscillatorTest::test_nextSample_saw_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0); // 100 samples per cycle
    osc.setWaveform(PolyBlepOscillator::Waveform::Saw);

    // Initial phase is 0.0
    // At t=0, saw value is (2*0)-1 = -1.0.
    // PolyBlep correction at t=0: polyBlep(0) = 0 + 0 - 0 - 1 = -1.0
    // value = -1.0 - (-1.0) = 0.0
    QCOMPARE(osc.nextSample(), 0.0);

    // After 25 samples, phase should be 0.25
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    // At t=0.25, saw value is (2*0.25)-1 = -0.5.
    // polyBlep(0.25) is 0 because dt = 441/44100 = 0.01
    QCOMPARE(osc.nextSample(), -0.5);

    // At t=0.5
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.0);

    // At t=0.75
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.5);
}

void PolyBlepOscillatorTest::test_nextSample_square_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0); // 100 samples per cycle
    osc.setWaveform(PolyBlepOscillator::Waveform::Square);

    // The pulse is not an ideal one: it comes out of a stage with a finite bandwidth and an AC
    // coupling, so the corners are rounded and the flat parts sag towards zero. Exact sample values
    // would only restate those two filters, so what is checked here is the shape they leave: the
    // wave sits at the rails between the edges, and crosses zero where the edges are.
    std::vector<double> cycle;
    for (int i = 0; i < 200; i++) {
        const double sample = osc.nextSample();
        if (i >= 100) { // Second cycle: the first is the coupling settling
            cycle.push_back(sample);
        }
    }

    // Mid-way through each half, well clear of both edges
    QVERIFY(cycle.at(25) > 0.9);
    QVERIFY(cycle.at(75) < -0.9);

    // The edges take several samples to cross rather than stepping across in one. An ideal square
    // moves the whole 2.0 between two samples; this one takes about four to get there.
    double maxStep = 0.0;
    for (size_t i = 1; i < cycle.size(); i++) {
        maxStep = std::max(maxStep, std::abs(cycle.at(i) - cycle.at(i - 1)));
    }
    QVERIFY(maxStep > 0.1); // ...but it is still an edge, not a sine
    QVERIFY(maxStep < 1.2);

    // Nothing runs away past the rails
    for (const double sample : cycle) {
        QVERIFY(std::abs(sample) < 1.1);
    }
}

void PolyBlepOscillatorTest::test_square_flatPartsShouldSagTowardsZero()
{
    // The dip along the top of the wave, which is the coupling letting the level fall away while
    // nothing is driving it. Without it the tops are flat and the wave looks drawn rather than
    // measured.
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(110.0); // 401 samples per cycle: a long flat part to sag along
    osc.setWaveform(PolyBlepOscillator::Waveform::Square);

    std::vector<double> cycle;
    for (int i = 0; i < 4000; i++) {
        const double sample = osc.nextSample();
        if (i >= 3600) {
            cycle.push_back(sample);
        }
    }

    // Along one flat part, away from either edge
    const double early = cycle.at(40);
    const double late = cycle.at(160);
    QVERIFY(early > 0.0);
    QVERIFY(late > 0.0);
    QVERIFY(late < early); // ...and it has fallen on the way
}

void PolyBlepOscillatorTest::test_square_shapeShouldNotChangeTheLevel()
{
    // Shape is a timbre control. A pulse swings between the same two rails whatever its duty, so
    // without normalisation a thin one stands nearly twice as far from zero as a half-duty one and
    // arrives at the filter some 5 dB hotter -- which is what made a thin pulse piercing.
    const auto peak = [](double shape) {
        PolyBlepOscillator osc;
        osc.setSampleRate(44100.0);
        osc.setFrequency(220.0);
        osc.setWaveform(PolyBlepOscillator::Waveform::Square);
        osc.setShape(shape);
        double result = 0.0;
        for (int i = 0; i < 4000; i++) {
            const double sample = osc.nextSample();
            if (i >= 2000) {
                result = std::max(result, std::abs(sample));
            }
        }
        return result;
    };

    const double wide = peak(0.0);
    const double narrow = peak(1.0);

    QVERIFY(wide > 0.9 && wide < 1.15);
    QVERIFY(narrow > 0.4 && narrow <= wide);
}

void PolyBlepOscillatorTest::test_nextSample_triangle_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);
    osc.setWaveform(PolyBlepOscillator::Waveform::Triangle);

    // t=0, value = (4*0 - 1) = -1.0
    QCOMPARE(osc.nextSample(), -1.0);

    // t=0.25, value = (4*0.25 - 1) = 0.0
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.0);

    // t=0.5, value = (3 - 4*0.5) = 1.0
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 1.0);
}

void PolyBlepOscillatorTest::test_nextSample_triangle_fold_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0); // dt = 0.01
    osc.setWaveform(PolyBlepOscillator::Waveform::Triangle);
    osc.setShape(0.25); // (1 + 0.25*4) = 2.0 multiplier

    // t=0, original value -1.0. Multiplied by 2.0 = -2.0.
    // Fold: -2.0 < -1.0 => -2.0 - (-2.0) = 0.0.
    QCOMPARE(osc.nextSample(), 0.0); // t=0.0, next is t=0.01

    // At t=0.1, original triangle value = 4*0.1 - 1 = -0.6.
    // Multiplied by 2.0 = -1.2.
    // Folded: -2.0 - (-1.2) = -0.8.
    for (int i = 0; i < 9; i++) {
        osc.nextSample();
    }
    // After loop, 10 samples total have been processed.
    // Next call is 11th sample, t=0.10.
    QCOMPARE(osc.nextSample(), -0.8);

    // At t=0.25, original triangle value = 0.0.
    // Multiplied by 2.0 = 0.0.
    for (int i = 0; i < 14; i++) {
        osc.nextSample();
    }
    // 11 + 14 = 25 samples total.
    // Next call is 26th sample, t=0.25.
    QCOMPARE(osc.nextSample(), 0.0);
}

void PolyBlepOscillatorTest::test_nextSample_sine_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);
    osc.setWaveform(PolyBlepOscillator::Waveform::Sine);

    // t=0, sine(0) = 0.0
    QCOMPARE(osc.nextSample(), 0.0);

    // At t=0.25, sine(2*pi*0.25) = sine(pi/2) = 1.0
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 1.0);

    // At t=0.5, sine(pi) = 0.0
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.0);
}

void PolyBlepOscillatorTest::test_nextSample_sine_fold_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0); // dt = 0.01
    osc.setWaveform(PolyBlepOscillator::Waveform::Sine);
    osc.setShape(0.25); // multiplier = 2.0

    // t=0, sine=0. Fold=0. Next is 0.01
    QCOMPARE(osc.nextSample(), 0.0);

    // At t=0.1
    // sine(2*pi*0.1) approx 0.587785
    // Multiplied by 2.0 approx 1.17557
    // Folded: 2.0 - 1.17557 approx 0.82443
    for (int i = 0; i < 9; i++) {
        osc.nextSample();
    }
    double expected = 2.0 - 2.0 * std::sin(std::numbers::pi * 2.0 * 0.1);
    QVERIFY(std::abs(osc.nextSample() - expected) < 0.001);

    // At t=0.25, sine = 1.0. Multiplied by 2.0 = 2.0.
    // Folded: 2.0 - 2.0 = 0.0
    for (int i = 0; i < 14; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.0);
}

void PolyBlepOscillatorTest::test_setFrequency_shouldUpdatePhaseStep()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);

    osc.nextSample();
    QCOMPARE(osc.phase(), 0.01);

    osc.setFrequency(882.0);
    osc.nextSample();
    QCOMPARE(osc.phase(), 0.03);
}

void PolyBlepOscillatorTest::test_sync_shouldResetPhase()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);

    osc.nextSample();
    osc.nextSample();
    QVERIFY(osc.phase() > 0.0);

    osc.sync(0.5);
    QCOMPARE(osc.phase(), 0.5);
}

void PolyBlepOscillatorTest::test_setPulseWidth_shouldClampValues()
{
    PolyBlepOscillator osc;

    osc.setPulseWidth(0.5);
    QCOMPARE(osc.pulseWidth(), 0.5);

    osc.setPulseWidth(-1.0);
    QCOMPARE(osc.pulseWidth(), 0.01);

    osc.setPulseWidth(2.0);
    QCOMPARE(osc.pulseWidth(), 0.99);
}

void PolyBlepOscillatorTest::test_setShape_shouldClampValues()
{
    PolyBlepOscillator osc;

    osc.setShape(0.5);
    QCOMPARE(osc.shape(), 0.5);

    osc.setShape(-1.0);
    QCOMPARE(osc.shape(), 0.0);

    osc.setShape(2.0);
    QCOMPARE(osc.shape(), 1.0);
}

void PolyBlepOscillatorTest::test_pulseWidth_shouldAffectOutput()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0); // 100 samples per period
    osc.setWaveform(PolyBlepOscillator::Waveform::Square);

    // Render with 50% pulse width
    osc.setPulseWidth(0.5);
    osc.sync(0.0);
    std::vector<double> samples50;
    for (int i = 0; i < 100; ++i) {
        samples50.push_back(osc.nextSample());
    }

    // Render with 25% pulse width
    osc.setPulseWidth(0.25);
    osc.sync(0.0);
    std::vector<double> samples25;
    for (int i = 0; i < 100; ++i) {
        samples25.push_back(osc.nextSample());
    }

    // The output arrays should not be identical
    QVERIFY(samples50 != samples25);
}

void PolyBlepOscillatorTest::test_shape_shouldAffectSquareOutput()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);
    osc.setWaveform(PolyBlepOscillator::Waveform::Square);

    // Render with shape = 0.0 (50% pulse width)
    osc.setShape(0.0);
    osc.sync(0.0);
    std::vector<double> samplesShape0;
    for (int i = 0; i < 100; ++i) {
        samplesShape0.push_back(osc.nextSample());
    }

    // Render with shape = 0.5 (narrower pulse width)
    osc.setShape(0.5);
    osc.sync(0.0);
    std::vector<double> samplesShape05;
    for (int i = 0; i < 100; ++i) {
        samplesShape05.push_back(osc.nextSample());
    }

    // The outputs should not be identical
    QVERIFY(samplesShape0 != samplesShape05);
}

void PolyBlepOscillatorTest::test_shape_shouldAffectSawOutput()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);
    osc.setWaveform(PolyBlepOscillator::Waveform::Saw);

    // Render with shape = 0.0
    osc.setShape(0.0);
    osc.sync(0.0);
    std::vector<double> samplesShape0;
    for (int i = 0; i < 100; ++i) {
        samplesShape0.push_back(osc.nextSample());
    }

    // Render with shape = 0.5
    osc.setShape(0.5);
    osc.sync(0.0);
    std::vector<double> samplesShape05;
    for (int i = 0; i < 100; ++i) {
        samplesShape05.push_back(osc.nextSample());
    }

    // The outputs should not be identical
    QVERIFY(samplesShape0 != samplesShape05);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PolyBlepOscillatorTest)
