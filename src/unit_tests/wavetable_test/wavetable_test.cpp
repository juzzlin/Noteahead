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

#include "wavetable_test.hpp"

#include "../../domain/dsp/wavetable.hpp"

#include <QTest>
#include <algorithm>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 44100.0;

const std::vector<Wavetable::WavetableS> & allSets()
{
    static const auto sets = []() {
        std::vector<Wavetable::WavetableS> tables;
        for (size_t i = 0; i < Wavetable::setNames().size(); i++) {
            tables.push_back(Wavetable::createSet(i));
        }
        return tables;
    }();
    return sets;
}

} // namespace

void WavetableTest::test_setNames_ordinals_shouldStayStable()
{
    // The synth serializes the selection as an index into this list, so an entry may never move.
    // Anything new belongs at the end.
    const auto names = Wavetable::setNames();
    QVERIFY(names.size() >= 6);
    QCOMPARE(names.at(0), std::string { "Classic Morph" });
    QCOMPARE(names.at(1), std::string { "Spectral Additive" });
    QCOMPARE(names.at(2), std::string { "Pulse Width" });
    QCOMPARE(names.at(3), std::string { "Vocal Formant" });
    QCOMPARE(names.at(4), std::string { "Resonant Sweep" });
    QCOMPARE(names.at(5), std::string { "Organ Drawbar" });

    auto sorted = names;
    std::sort(sorted.begin(), sorted.end());
    QCOMPARE(std::adjacent_find(sorted.begin(), sorted.end()), sorted.end());
}

void WavetableTest::test_existingSets_shouldMatchThePreFftGenerator()
{
    // Captured from the additive generator that shipped before these sets were synthesized by FFT.
    // Songs written against those two sets have to keep sounding the way they were written.
    static const std::vector<float> expected = {
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.699961901f,
        0.699961901f,
        0.699961901f,
        -0.000000087f,
        -0.000000087f,
        -0.000000087f,
        -0.984891593f,
        -0.984891593f,
        -0.984891593f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.536574662f,
        0.539872766f,
        0.568703055f,
        -0.000000070f,
        -0.000000070f,
        -0.000000073f,
        -0.909697950f,
        -0.914356947f,
        -0.964392006f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.899848104f,
        0.884660542f,
        0.878826261f,
        -0.000000201f,
        -0.000000154f,
        -0.000000105f,
        -0.974463582f,
        -0.975130916f,
        -0.906883180f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.672649682f,
        0.666313529f,
        0.755987346f,
        -0.000000198f,
        -0.000000143f,
        -0.000000094f,
        -0.528793633f,
        -0.537084162f,
        -0.532487869f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.848757148f,
        0.833981395f,
        0.840318680f,
        -0.000000343f,
        -0.000000245f,
        -0.000000146f,
        -0.847407281f,
        -0.842684925f,
        -0.800118089f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.699961901f,
        0.699961901f,
        0.699961901f,
        -0.000000087f,
        -0.000000087f,
        -0.000000087f,
        -0.984891593f,
        -0.984891593f,
        -0.984891593f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.641135812f,
        0.641135812f,
        0.735381961f,
        -0.000000102f,
        -0.000000102f,
        -0.000000084f,
        -0.484353811f,
        -0.484353811f,
        -0.475062609f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.636755109f,
        0.635544717f,
        0.735381961f,
        -0.000000115f,
        -0.000000115f,
        -0.000000084f,
        -0.488252878f,
        -0.485780537f,
        -0.475062609f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.636158109f,
        0.635158360f,
        0.735381961f,
        -0.000000124f,
        -0.000000121f,
        -0.000000084f,
        -0.476063550f,
        -0.477892429f,
        -0.475062609f,
        0.000000000f,
        0.000000000f,
        0.000000000f,
        0.636708438f,
        0.635514498f,
        0.735381961f,
        -0.000000130f,
        -0.000000125f,
        -0.000000084f,
        -0.480839372f,
        -0.480983019f,
        -0.475062609f,
    };

    size_t index = 0;
    for (size_t set = 0; set < 2; set++) {
        const auto table = Wavetable::createSet(set);
        for (const double position : { 0.0, 0.25, 0.5, 0.75, 1.0 }) {
            for (const double phase : { 0.0, 0.1234, 0.5, 0.7777 }) {
                for (const double frequency : { 55.0, 440.0, 3520.0 }) {
                    const auto actual = table->getSample(phase, position, frequency, SampleRate);
                    const auto difference = std::abs(actual - expected.at(index));
                    QVERIFY2(difference < 1.0e-5f,
                             qPrintable(QString { "%1 at position %2, phase %3, %4 Hz: expected %5, got %6" }
                                          .arg(QString::fromStdString(table->name()))
                                          .arg(position)
                                          .arg(phase)
                                          .arg(frequency)
                                          .arg(static_cast<double>(expected.at(index)))
                                          .arg(static_cast<double>(actual))));
                    index++;
                }
            }
        }
    }
    QCOMPARE(index, expected.size());
}

void WavetableTest::test_allSets_shouldStayWithinFullScale()
{
    for (auto && table : allSets()) {
        for (int w = 0; w < Wavetable::NumWaves; w++) {
            const double position = static_cast<double>(w) / (Wavetable::NumWaves - 1);
            for (int i = 0; i < Wavetable::WaveSize; i++) {
                const auto sample = table->getSample(static_cast<double>(i) / Wavetable::WaveSize, position, 30.0, SampleRate);
                QVERIFY2(std::isfinite(sample), qPrintable(QString::fromStdString(table->name())));
                QVERIFY2(std::abs(sample) <= 1.0f,
                         qPrintable(QString { "%1 reaches %2 at position %3" }.arg(QString::fromStdString(table->name())).arg(static_cast<double>(sample)).arg(position)));
            }
        }
    }
}

void WavetableTest::test_rmsNormalizedSets_shouldHoldLevelAcrossTheMorph()
{
    // Sets built with Normalization::Rms exist so that sweeping Pos does not sweep the volume with
    // it. Some spread is unavoidable: a wave too spiky to reach the target loudness without
    // clipping is left at full scale instead, and a narrow pulse really is thinner than a square.
    // What this catches is a new set whose level runs away across the morph.
    for (size_t set = 2; set < Wavetable::setNames().size(); set++) {
        const auto table = allSets().at(set);
        double quietest = 1.0;
        double loudest = 0.0;
        for (int w = 0; w < Wavetable::NumWaves; w++) {
            const double position = static_cast<double>(w) / (Wavetable::NumWaves - 1);
            double sumSquares = 0.0;
            for (int i = 0; i < Wavetable::WaveSize; i++) {
                const auto sample = table->getSample(static_cast<double>(i) / Wavetable::WaveSize, position, 30.0, SampleRate);
                sumSquares += static_cast<double>(sample) * sample;
            }
            const auto rms = std::sqrt(sumSquares / Wavetable::WaveSize);
            quietest = std::min(quietest, rms);
            loudest = std::max(loudest, rms);
        }

        const auto spreadDb = 20.0 * std::log10(loudest / quietest);
        QVERIFY2(spreadDb < 9.0,
                 qPrintable(QString { "%1 spreads %2 dB across the morph" }.arg(QString::fromStdString(table->name())).arg(spreadDb)));
    }
}

void WavetableTest::test_pulseWidthSet_shouldNarrowAcrossTheMorph()
{
    const auto names = Wavetable::setNames();
    const auto index = static_cast<size_t>(std::distance(names.begin(), std::find(names.begin(), names.end(), std::string { "Pulse Width" })));
    const auto table = Wavetable::createSet(index);

    // The fraction of the cycle spent above zero is the duty cycle, and it has to shrink as the
    // morph advances or the set is not doing what it is named for.
    const auto dutyCycle = [&table](double position) {
        int above = 0;
        for (int i = 0; i < Wavetable::WaveSize; i++) {
            if (table->getSample(static_cast<double>(i) / Wavetable::WaveSize, position, 30.0, SampleRate) > 0.0f) {
                above++;
            }
        }
        return static_cast<double>(above) / Wavetable::WaveSize;
    };

    const auto square = dutyCycle(0.0);
    const auto middle = dutyCycle(0.5);
    const auto narrow = dutyCycle(1.0);

    QVERIFY2(std::abs(square - 0.5) < 0.02, qPrintable(QString { "Duty at the square end is %1" }.arg(square)));
    QVERIFY2(middle < square, qPrintable(QString { "Duty %1 did not narrow from %2" }.arg(middle).arg(square)));
    QVERIFY2(narrow < middle, qPrintable(QString { "Duty %1 did not narrow from %2" }.arg(narrow).arg(middle)));
}

void WavetableTest::test_harmonicsAboveNyquist_shouldBeBandLimitedAway()
{
    // The mip levels exist so that a high note does not fold its harmonics back down the spectrum.
    // A wave read at a high frequency must therefore be far simpler than the same wave read low.
    const auto zeroCrossings = [](const Wavetable::WavetableS & table, double frequency) {
        int crossings = 0;
        auto previous = table->getSample(0.0, 1.0, frequency, SampleRate);
        for (int i = 1; i < Wavetable::WaveSize; i++) {
            const auto sample = table->getSample(static_cast<double>(i) / Wavetable::WaveSize, 1.0, frequency, SampleRate);
            if ((previous < 0.0f) != (sample < 0.0f)) {
                crossings++;
            }
            previous = sample;
        }
        return crossings;
    };

    for (auto && table : allSets()) {
        const auto low = zeroCrossings(table, 30.0);
        const auto high = zeroCrossings(table, 8000.0);
        QVERIFY2(high <= low, qPrintable(QString { "%1 has %2 crossings high against %3 low" }.arg(QString::fromStdString(table->name())).arg(high).arg(low)));
        QVERIFY2(high <= 4, qPrintable(QString { "%1 still has %2 crossings at 8 kHz" }.arg(QString::fromStdString(table->name())).arg(high)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WavetableTest)
