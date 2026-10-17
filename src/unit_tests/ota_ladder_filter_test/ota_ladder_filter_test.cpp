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

#include "ota_ladder_filter_test.hpp"

#include "../../domain/dsp/ota_ladder_filter.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

//! Steady-state gain for a quiet sine, so the OTAs stay linear.
double gainAt(double cutoff, double resonance, double hz)
{
    OtaLadderFilter filter;
    filter.setSampleRate(SampleRate);
    filter.setCutoff(cutoff);
    filter.setResonance(resonance);
    constexpr double amplitude { 0.01 };
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

} // namespace

void OtaLadderFilterTest::test_process_belowCutoff_shouldPassAtUnity()
{
    const double gain = gainAt(2000.0, 0.0, 100.0);
    QVERIFY2(std::abs(gain - 1.0) < 0.02, qPrintable(QString::number(gain)));
}

void OtaLadderFilterTest::test_process_twoOctavesAbove_shouldBeDownBy24DbPerOctave()
{
    // Four poles: well past the corner each octave takes 24 dB, so two take about 48
    const double db = 20.0 * std::log10(gainAt(1000.0, 0.0, 4000.0));
    QVERIFY2(db < -40.0 && db > -56.0, qPrintable(QString::number(db)));
}

void OtaLadderFilterTest::test_resonance_full_shouldSelfOscillateAtTheCutoff()
{
    OtaLadderFilter filter;
    filter.setSampleRate(SampleRate);
    filter.setCutoff(1000.0);
    filter.setResonance(1.0);
    // One kick, then nothing in: whatever comes out after that the filter makes itself
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
    QVERIFY2(peak > 0.1, qPrintable(QString::number(peak)));
    QVERIFY2(std::abs(crossings - 1000) < 50, qPrintable(QString::number(crossings)));
}

void OtaLadderFilterTest::test_resonance_half_shouldDieAway()
{
    OtaLadderFilter filter;
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

void OtaLadderFilterTest::test_resonance_raised_shouldThinTheBassOnlySlightly()
{
    // A Juno-106 loses about 1.2 dB of bass to a resonance that lifts the highs by 10 dB
    const double open = gainAt(2000.0, 0.0, 100.0);
    const double resonant = gainAt(2000.0, 0.85, 100.0);
    QVERIFY2(resonant < 0.95 * open, qPrintable(QString { "%1 vs %2" }.arg(resonant).arg(open)));
    QVERIFY2(resonant > 0.75 * open, qPrintable(QString { "%1 vs %2" }.arg(resonant).arg(open)));
}

void OtaLadderFilterTest::test_process_loudInput_shouldStayBounded()
{
    OtaLadderFilter filter;
    filter.setSampleRate(SampleRate);
    filter.setCutoff(500.0);
    filter.setResonance(1.0);
    for (int i = 0; i < static_cast<int>(SampleRate); i++) {
        const double output = filter.process(10.0 * std::sin(2.0 * M_PI * 110.0 * i / SampleRate));
        QVERIFY(std::isfinite(output));
        QVERIFY(std::abs(output) < 2.0);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::OtaLadderFilterTest)
