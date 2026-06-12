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

#include "lfo_test.hpp"

#include "domain/dsp/lfo.hpp"

#include <QTest>
#include <algorithm>
#include <vector>

namespace noteahead {

namespace {

// Returns an Lfo at 44100 Hz with a near-zero phase step so setPhase() tests are clean.
Lfo createLfo(Lfo::Waveform waveform)
{
    Lfo lfo;
    lfo.setSampleRate(44100.0);
    lfo.setFrequency(0.001);
    lfo.setWaveform(waveform);
    lfo.reset();
    return lfo;
}

} // namespace

void LfoTest::test_waveformNames_shouldContainFiveEntries()
{
    const auto names = Lfo::waveformNames();
    QCOMPARE(static_cast<int>(names.size()), 5);
    QVERIFY(std::find(names.begin(), names.end(), "Random") != names.end());
}

// --- Sine ---

void LfoTest::test_sineWaveform_shouldOutputZeroAtPhaseZero()
{
    auto lfo = createLfo(Lfo::Waveform::Sine);
    lfo.setPhase(0.0);
    QVERIFY(std::abs(lfo.nextSample()) < 1e-9);
}

void LfoTest::test_sineWaveform_shouldOutputOneAtQuarterCycle()
{
    auto lfo = createLfo(Lfo::Waveform::Sine);
    lfo.setPhase(0.25);
    QVERIFY(std::abs(lfo.nextSample() - 1.0) < 1e-9);
}

// --- Saw ---

void LfoTest::test_sawWaveform_shouldStartAtNegativeOne()
{
    auto lfo = createLfo(Lfo::Waveform::Saw);
    lfo.setPhase(0.0);
    QVERIFY(std::abs(lfo.nextSample() - (-1.0)) < 1e-9);
}

void LfoTest::test_sawWaveform_shouldRampToPositiveOne()
{
    auto lfo = createLfo(Lfo::Waveform::Saw);
    lfo.setPhase(1.0 - 1e-9);
    QVERIFY(lfo.nextSample() > 0.99);
}

// --- Triangle ---

void LfoTest::test_triangleWaveform_shouldStartAtNegativeOne()
{
    auto lfo = createLfo(Lfo::Waveform::Triangle);
    lfo.setPhase(0.0);
    QVERIFY(std::abs(lfo.nextSample() - (-1.0)) < 1e-9);
}

void LfoTest::test_triangleWaveform_shouldPeakAtMidCycle()
{
    auto lfo = createLfo(Lfo::Waveform::Triangle);
    lfo.setPhase(0.5);
    QVERIFY(std::abs(lfo.nextSample() - 1.0) < 1e-9);
}

// --- Square ---

void LfoTest::test_squareWaveform_shouldOutputOneInFirstHalf()
{
    auto lfo = createLfo(Lfo::Waveform::Square);
    lfo.setPhase(0.0);
    QCOMPARE(lfo.nextSample(), 1.0);
    lfo.setPhase(0.25);
    QCOMPARE(lfo.nextSample(), 1.0);
}

void LfoTest::test_squareWaveform_shouldOutputNegativeOneInSecondHalf()
{
    auto lfo = createLfo(Lfo::Waveform::Square);
    lfo.setPhase(0.5);
    QCOMPARE(lfo.nextSample(), -1.0);
    lfo.setPhase(0.75);
    QCOMPARE(lfo.nextSample(), -1.0);
}

// --- Modes ---

void LfoTest::test_oneShotMode_shouldStopAfterOneCycle()
{
    Lfo lfo;
    lfo.setSampleRate(100.0);
    lfo.setFrequency(1.0); // 100 samples per cycle
    lfo.setWaveform(Lfo::Waveform::Square);
    lfo.setMode(Lfo::Mode::OneShot);
    lfo.reset();

    for (int i = 0; i < 100; i++) {
        lfo.nextSample();
    }

    for (int i = 0; i < 100; i++) {
        QCOMPARE(lfo.nextSample(), 0.0);
    }
}

void LfoTest::test_normalMode_shouldContinueAcrossMultipleCycles()
{
    Lfo lfo;
    lfo.setSampleRate(100.0);
    lfo.setFrequency(1.0); // 100 samples per cycle
    lfo.setWaveform(Lfo::Waveform::Square);
    lfo.setMode(Lfo::Mode::Normal);
    lfo.reset();

    // Skip 3 full cycles
    for (int i = 0; i < 300; i++) {
        lfo.nextSample();
    }

    // Square wave is always ±1, so any sample in the 4th cycle must be non-zero
    QVERIFY(std::abs(lfo.nextSample()) > 0.5);
}

// --- Phase & frequency ---

void LfoTest::test_setPhase_shouldAffectOutput()
{
    auto lfo = createLfo(Lfo::Waveform::Sine);

    lfo.setPhase(0.0);
    const double atZero = lfo.nextSample();

    lfo.setPhase(0.25);
    const double atQuarter = lfo.nextSample();

    QVERIFY(std::abs(atZero) < 1e-9); // sin(0) = 0
    QVERIFY(std::abs(atQuarter - 1.0) < 1e-9); // sin(π/2) = 1
}

void LfoTest::test_higherFrequency_shouldProduceShorterCycle()
{
    auto cycleLength = [](double freq) -> int {
        Lfo lfo;
        lfo.setSampleRate(10000.0);
        lfo.setFrequency(freq);
        lfo.setWaveform(Lfo::Waveform::Saw);
        lfo.reset();
        double prev = lfo.nextSample();
        for (int i = 1; i < 1000000; i++) {
            const double curr = lfo.nextSample();
            if (curr < prev - 1.0) {
                return i;
            }
            prev = curr;
        }
        return -1;
    };

    const int slow = cycleLength(1.0);
    const int fast = cycleLength(10.0);
    QVERIFY(slow > 0);
    QVERIFY(fast > 0);
    QVERIFY(slow > fast);
}

// --- Random ---

void LfoTest::test_randomWaveform_shouldProduceValuesInRange()
{
    Lfo lfo;
    lfo.setSampleRate(44100.0);
    lfo.setFrequency(1.0);
    lfo.setWaveform(Lfo::Waveform::Random);
    lfo.reset();

    for (int i = 0; i < 44100; i++) {
        const double v = lfo.nextSample();
        QVERIFY(v >= -1.0);
        QVERIFY(v <= 1.0);
    }
}

void LfoTest::test_randomWaveform_shouldHoldValueForOneCycle()
{
    Lfo lfo;
    lfo.setSampleRate(100.0);
    lfo.setFrequency(1.0); // 100 samples per cycle
    lfo.setWaveform(Lfo::Waveform::Random);
    lfo.reset();

    const double firstCycleValue = lfo.nextSample();
    for (int i = 1; i < 100; i++) {
        QCOMPARE(lfo.nextSample(), firstCycleValue);
    }

    const double secondCycleValue = lfo.nextSample();
    for (int i = 1; i < 100; i++) {
        QCOMPARE(lfo.nextSample(), secondCycleValue);
    }
}

void LfoTest::test_randomWaveform_shouldBeDeterministicAfterReset()
{
    Lfo lfo;
    lfo.setSampleRate(44100.0);
    lfo.setFrequency(440.0);
    lfo.setWaveform(Lfo::Waveform::Random);
    lfo.reset();

    std::vector<double> firstRun;
    firstRun.reserve(1000);
    for (int i = 0; i < 1000; i++) {
        firstRun.push_back(lfo.nextSample());
    }

    lfo.reset();
    for (size_t i = 0; i < 1000; i++) {
        QCOMPARE(lfo.nextSample(), firstRun[i]);
    }
}

// --- BPM frequency overload ---

namespace {

// Returns how many samples until the saw waveform wraps (one full cycle).
int sawCycleLength(double sampleRate, double frequency)
{
    Lfo lfo;
    lfo.setSampleRate(sampleRate);
    lfo.setFrequency(frequency);
    lfo.setWaveform(Lfo::Waveform::Saw);
    lfo.reset();
    double prev = lfo.nextSample();
    for (int i = 1; i < 1'000'000; i++) {
        const double curr = lfo.nextSample();
        if (curr < prev - 1.0)
            return i;
        prev = curr;
    }
    return -1;
}

int sawCycleLengthBpm(double sampleRate, double bpm, double syncRate)
{
    Lfo lfo;
    lfo.setSampleRate(sampleRate);
    lfo.setFrequency(bpm, syncRate);
    lfo.setWaveform(Lfo::Waveform::Saw);
    lfo.reset();
    double prev = lfo.nextSample();
    for (int i = 1; i < 1'000'000; i++) {
        const double curr = lfo.nextSample();
        if (curr < prev - 1.0)
            return i;
        prev = curr;
    }
    return -1;
}

} // namespace

void LfoTest::test_setFrequency_bpm_shouldProduceCycleMatchingBpm()
{
    // 120 BPM, syncRate=0.25 (quarter note) → (120/60)*(0.25/0.25) = 2 Hz
    // At sr=100: cycle = 50 samples. Direct setFrequency(2.0) must give the same length.
    const double sr = 100.0;
    const int direct = sawCycleLength(sr, 2.0);
    const int viaBpm = sawCycleLengthBpm(sr, 120.0, 0.25);
    QCOMPARE(viaBpm, direct);
}

void LfoTest::test_setFrequency_bpm_higherBpm_shouldProduceShorterCycle()
{
    const double sr = 10000.0;
    const int slow = sawCycleLengthBpm(sr, 60.0, 0.25);
    const int fast = sawCycleLengthBpm(sr, 120.0, 0.25);
    QVERIFY(slow > 0);
    QVERIFY(fast > 0);
    QVERIFY(std::abs(slow - fast * 2) <= 2);
}

void LfoTest::test_setFrequency_bpm_differentSyncRate_shouldScaleCycle()
{
    // Halving syncRate halves the frequency → doubles the cycle length.
    const double sr = 10000.0;
    const int quarter = sawCycleLengthBpm(sr, 120.0, 0.25);
    const int half = sawCycleLengthBpm(sr, 120.0, 0.5);
    QVERIFY(quarter > 0);
    QVERIFY(half > 0);
    QVERIFY(std::abs(half - quarter * 2) <= 2);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LfoTest)
