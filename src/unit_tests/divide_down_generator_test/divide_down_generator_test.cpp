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

#include "divide_down_generator_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/divide_down_generator.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 44100.0 };

std::vector<double> render(DivideDownGenerator & generator, uint8_t note, int octaveOffset, int frames)
{
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(frames));
    for (int i = 0; i < frames; i++) {
        generator.tick();
        samples.push_back(generator.saw(note, octaveOffset));
    }
    return samples;
}

//! Frequency estimated from the rising zero crossings, which a saw makes exactly once per cycle.
//! The band-limiting only touches the phase wrap, so the crossing at mid-cycle stays clean.
double estimateFrequency(const std::vector<double> & samples)
{
    int crossings = 0;
    for (size_t i = 1; i < samples.size(); i++) {
        if (samples[i - 1] <= 0.0 && samples[i] > 0.0) {
            crossings++;
        }
    }
    return static_cast<double>(crossings) * SampleRate / static_cast<double>(samples.size());
}

//! Windowed magnitude sum below the given frequency. Hann keeps the fundamental from leaking into
//! the low bins, so what is left down there is aliasing and nothing else.
double energyBelow(const std::vector<double> & samples, double frequency)
{
    const int frames = static_cast<int>(samples.size());
    std::vector<double> windowed(samples.size());
    for (int i = 0; i < frames; i++) {
        const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (frames - 1));
        windowed[static_cast<size_t>(i)] = samples[static_cast<size_t>(i)] * window;
    }

    const int maxBin = static_cast<int>(frequency * frames / SampleRate);
    double total = 0.0;
    for (int bin = 1; bin < maxBin; bin++) {
        double re = 0.0;
        double im = 0.0;
        for (int i = 0; i < frames; i++) {
            const double angle = 2.0 * M_PI * bin * i / frames;
            re += windowed[static_cast<size_t>(i)] * std::cos(angle);
            im -= windowed[static_cast<size_t>(i)] * std::sin(angle);
        }
        total += std::sqrt(re * re + im * im);
    }
    return total / frames;
}

} // namespace

void DivideDownGeneratorTest::test_saw_octaveTaps_shouldStayPhaseLocked()
{
    // The whole point of a divider chain: a key's 4' tap and the key an octave above it at 8' are
    // the same signal, sample for sample, because both come out of one master phasor.
    DivideDownGenerator generator;
    generator.setSampleRate(SampleRate);

    for (int i = 0; i < 1000; i++) {
        generator.tick();
        QCOMPARE(generator.saw(48, 1), generator.saw(60, 0));
        QCOMPARE(generator.saw(48, -1), generator.saw(36, 0));
    }
}

void DivideDownGeneratorTest::test_saw_octaveTaps_shouldDoubleTheFrequency()
{
    DivideDownGenerator generator;
    generator.setSampleRate(SampleRate);

    const auto eightFoot = render(generator, 60, 0, static_cast<int>(SampleRate));
    generator.reset();
    const auto fourFoot = render(generator, 60, 1, static_cast<int>(SampleRate));
    generator.reset();
    const auto sixteenFoot = render(generator, 60, -1, static_cast<int>(SampleRate));

    QVERIFY(std::abs(estimateFrequency(fourFoot) - 2.0 * estimateFrequency(eightFoot)) < 2.0);
    QVERIFY(std::abs(estimateFrequency(sixteenFoot) - 0.5 * estimateFrequency(eightFoot)) < 2.0);
}

void DivideDownGeneratorTest::test_saw_note_shouldMatchEqualTemperament()
{
    DivideDownGenerator generator;
    generator.setSampleRate(SampleRate);

    // A4 = note 69 = 440 Hz at 8'
    const auto samples = render(generator, 69, 0, static_cast<int>(SampleRate));

    QVERIFY(std::abs(estimateFrequency(samples) - 440.0) < 2.0);
}

void DivideDownGeneratorTest::test_saw_defaultSampleRate_shouldStillAdvance()
{
    // Setting the rate a component already has is a no-op, so a generator left at the default must
    // already be running. Getting this wrong leaves every phasor frozen and the output at DC.
    const double defaultSampleRate = Constants::defaultSampleRate();

    DivideDownGenerator generator;
    generator.setSampleRate(defaultSampleRate);

    std::vector<double> samples;
    for (int i = 0; i < static_cast<int>(defaultSampleRate); i++) {
        generator.tick();
        samples.push_back(generator.saw(69, 0));
    }

    int crossings = 0;
    for (size_t i = 1; i < samples.size(); i++) {
        if (samples[i - 1] <= 0.0 && samples[i] > 0.0) {
            crossings++;
        }
    }

    const double frequency = static_cast<double>(crossings) * defaultSampleRate / static_cast<double>(samples.size());
    QVERIFY2(std::abs(frequency - 440.0) < 2.0, QString("A4 came out at %1 Hz").arg(frequency).toUtf8().constData());
}

void DivideDownGeneratorTest::test_saw_aboveNyquist_shouldBeSilent()
{
    DivideDownGenerator generator;
    generator.setSampleRate(SampleRate);

    // The top of the keyboard at 4' lands beyond what the sample rate can carry, so the tap is
    // dropped rather than left to alias.
    for (int i = 0; i < 100; i++) {
        generator.tick();
        QCOMPARE(generator.saw(127, 1), 0.0);
    }
}

void DivideDownGeneratorTest::test_saw_highOctave_shouldStayBandLimited()
{
    DivideDownGenerator generator;
    generator.setSampleRate(SampleRate);

    // Note 96 at 4' is 4186 Hz: high enough that partials fold back down below the fundamental.
    // Measured against a naive saw of the same frequency, which is what the generator would be
    // without its polyBLEP correction, the folded content has to be substantially lower.
    constexpr int Frames { 8192 };
    const auto bandLimited = render(generator, 96, 1, Frames);

    const double frequency = 440.0 * std::pow(2.0, (96.0 + 12.0 - 69.0) / 12.0);
    const double increment = frequency / SampleRate;
    std::vector<double> naive;
    naive.reserve(Frames);
    double phase = 0.0;
    for (int i = 0; i < Frames; i++) {
        phase += increment;
        if (phase >= 1.0) {
            phase -= 1.0;
        }
        naive.push_back(2.0 * phase - 1.0);
    }

    const double bandLimitedAliasing = energyBelow(bandLimited, 2000.0);
    const double naiveAliasing = energyBelow(naive, 2000.0);

    QVERIFY2(bandLimitedAliasing < naiveAliasing * 0.5,
             QString("Alias floor %1 vs naive %2").arg(bandLimitedAliasing).arg(naiveAliasing).toUtf8().constData());
}

void DivideDownGeneratorTest::test_saw_shouldStayBounded()
{
    DivideDownGenerator generator;
    generator.setSampleRate(SampleRate);

    for (int i = 0; i < 2000; i++) {
        generator.tick();
        for (uint8_t note = 0; note < 120; note += 7) {
            for (int octaveOffset = -1; octaveOffset <= 1; octaveOffset++) {
                const double sample = generator.saw(note, octaveOffset);
                QVERIFY(std::isfinite(sample));
                QVERIFY(std::abs(sample) <= 1.5);
            }
        }
    }
}

void DivideDownGeneratorTest::test_reset_shouldRestartAllPhases()
{
    DivideDownGenerator generator;
    generator.setSampleRate(SampleRate);

    const auto before = render(generator, 60, 0, 512);
    generator.reset();
    const auto after = render(generator, 60, 0, 512);

    QCOMPARE(before, after);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DivideDownGeneratorTest)
