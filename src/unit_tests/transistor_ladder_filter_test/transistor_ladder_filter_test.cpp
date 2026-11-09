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

#include "transistor_ladder_filter_test.hpp"

#include "../../domain/dsp/transistor_ladder_filter.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };
using Mode = TransistorLadderFilter::Mode;

//! Steady-state gain for a sine, quiet by default so the transistors stay linear
double gainAt(double cutoff, double resonance, double hz, Mode mode = Mode::LowPass, double amplitude = 0.01)
{
    TransistorLadderFilter filter;
    filter.setSampleRate(SampleRate);
    filter.setMode(mode);
    filter.setCutoff(cutoff);
    filter.setResonance(resonance);
    double peak = 0.0;
    const int frames = static_cast<int>(SampleRate);
    for (int i = 0; i < frames; i++) {
        const double output = filter.process(amplitude * std::sin(2.0 * M_PI * hz * i / SampleRate));
        if (i > frames / 2) {
            peak = std::max(peak, std::abs(output));
        }
    }
    return peak / amplitude;
}

//! Zero crossings per second and the peak of what the filter makes on its own after one kick
std::pair<int, double> ringAfterKick(Mode mode)
{
    TransistorLadderFilter filter;
    filter.setSampleRate(SampleRate);
    filter.setMode(mode);
    filter.setCutoff(1000.0);
    filter.setResonance(1.0);
    filter.process(0.1);
    int crossings = 0;
    double previous = 0.0;
    double peak = 0.0;
    for (int i = 0; i < static_cast<int>(2.0 * SampleRate); i++) {
        const double output = filter.process(0.0);
        if (i >= static_cast<int>(SampleRate)) {
            if (previous < 0.0 && output >= 0.0) {
                crossings++;
            }
            peak = std::max(peak, std::abs(output));
        }
        previous = output;
    }
    return { crossings, peak };
}

} // namespace

void TransistorLadderFilterTest::test_process_belowCutoff_shouldPassAtUnity()
{
    const double gain = gainAt(2000.0, 0.0, 100.0);
    QVERIFY2(std::abs(gain - 1.0) < 0.02, qPrintable(QString::number(gain)));
}

void TransistorLadderFilterTest::test_process_atCutoff_shouldBeDownBy12Db()
{
    // Four poles at one corner, 3 dB each
    const double db = 20.0 * std::log10(gainAt(1000.0, 0.0, 1000.0));
    QVERIFY2(std::abs(db + 12.0) < 0.5, qPrintable(QString::number(db)));
}

void TransistorLadderFilterTest::test_process_twoOctavesAbove_shouldBeDownBy24DbPerOctave()
{
    const double db = 20.0 * std::log10(gainAt(1000.0, 0.0, 4000.0));
    QVERIFY2(db < -40.0 && db > -56.0, qPrintable(QString::number(db)));
}

void TransistorLadderFilterTest::test_process_highPass_shouldMirrorTheLowPass()
{
    const double above = gainAt(1000.0, 0.0, 8000.0, Mode::HighPass);
    QVERIFY2(std::abs(above - 1.0) < 0.05, qPrintable(QString::number(above)));
    const double corner = 20.0 * std::log10(gainAt(1000.0, 0.0, 1000.0, Mode::HighPass));
    QVERIFY2(std::abs(corner + 12.0) < 0.5, qPrintable(QString::number(corner)));
    const double below = 20.0 * std::log10(gainAt(1000.0, 0.0, 250.0, Mode::HighPass));
    QVERIFY2(below < -40.0 && below > -56.0, qPrintable(QString::number(below)));
}

void TransistorLadderFilterTest::test_resonance_full_shouldSelfOscillateAtTheCutoff()
{
    const auto [crossings, peak] = ringAfterKick(Mode::LowPass);
    QVERIFY2(peak > 0.1, qPrintable(QString::number(peak)));
    QVERIFY2(std::abs(crossings - 1000) < 20, qPrintable(QString::number(crossings)));
}

void TransistorLadderFilterTest::test_resonance_fullHighPass_shouldSelfOscillateAtTheCutoff()
{
    const auto [crossings, peak] = ringAfterKick(Mode::HighPass);
    QVERIFY2(peak > 0.1, qPrintable(QString::number(peak)));
    QVERIFY2(std::abs(crossings - 1000) < 20, qPrintable(QString::number(crossings)));
}

void TransistorLadderFilterTest::test_resonance_half_shouldDieAway()
{
    TransistorLadderFilter filter;
    filter.setSampleRate(SampleRate);
    filter.setCutoff(1000.0);
    filter.setResonance(0.5);
    filter.process(0.1);
    double peak = 0.0;
    for (int i = 0; i < static_cast<int>(SampleRate); i++) {
        const double output = filter.process(0.0);
        if (i > static_cast<int>(0.5 * SampleRate)) {
            peak = std::max(peak, std::abs(output));
        }
    }
    QVERIFY2(peak < 1.0e-6, qPrintable(QString::number(peak)));
}

void TransistorLadderFilterTest::test_resonance_raised_shouldThinTheBassUncompensated()
{
    // Unlike the Juno's, nothing gives the passband back: it falls to 1 / (1 + feedback)
    constexpr double resonance = 0.5;
    const double expected = 1.0 / (1.0 + resonance * TransistorLadderFilter::MaxFeedback);
    const double ratio = gainAt(2000.0, resonance, 50.0) / gainAt(2000.0, 0.0, 50.0);
    QVERIFY2(std::abs(ratio - expected) < 0.02, qPrintable(QString { "%1 vs %2" }.arg(ratio).arg(expected)));
}

void TransistorLadderFilterTest::test_process_loudInput_shouldRoundOffHarmonically()
{
    // Pushed hard, the gain drops: the transistors compress rather than pass the level on
    const double quiet = gainAt(4000.0, 0.0, 100.0, Mode::LowPass, 0.01);
    const double loud = gainAt(4000.0, 0.0, 100.0, Mode::LowPass, 3.0);
    QVERIFY2(loud < 0.5 * quiet, qPrintable(QString { "%1 vs %2" }.arg(loud).arg(quiet)));
}

void TransistorLadderFilterTest::test_process_loudInput_shouldStayBounded()
{
    for (const auto mode : { Mode::LowPass, Mode::HighPass }) {
        TransistorLadderFilter filter;
        filter.setSampleRate(SampleRate);
        filter.setMode(mode);
        filter.setCutoff(500.0);
        filter.setResonance(1.0);
        for (int i = 0; i < static_cast<int>(SampleRate); i++) {
            const double output = filter.process(10.0 * std::sin(2.0 * M_PI * 110.0 * i / SampleRate));
            QVERIFY(std::isfinite(output));
            QVERIFY(std::abs(output) < 4.0);
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TransistorLadderFilterTest)
