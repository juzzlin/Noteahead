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

#include "micro_pitch_shifter_test.hpp"

#include "../../domain/dsp/micro_pitch_shifter.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;
constexpr double inputHz = 500.0;

//! Long enough for a reliable frequency estimate, and long enough that the crossfade runs at least
//! once for the shifts under test.
constexpr int frameCount = static_cast<int>(sampleRate) * 4;

//! The line has to fill before what comes out of it means anything.
constexpr int settleFrames = 8192;

//! Frequency estimated from the rising zero crossings, interpolated so the estimate is not limited
//! to whole samples.
double measureFrequency(const std::vector<double> & signal, int skip)
{
    double firstCrossing = -1.0;
    double lastCrossing = -1.0;
    int crossings = 0;

    for (size_t i = static_cast<size_t>(skip) + 1; i < signal.size(); i++) {
        const double previous = signal[i - 1];
        const double current = signal[i];
        if (previous < 0.0 && current >= 0.0) {
            const double fraction = -previous / (current - previous);
            const double position = static_cast<double>(i - 1) + fraction;
            if (firstCrossing < 0.0) {
                firstCrossing = position;
            }
            lastCrossing = position;
            crossings++;
        }
    }

    if (crossings < 2) {
        return 0.0;
    }

    return (crossings - 1) * sampleRate / (lastCrossing - firstCrossing);
}

std::vector<double> render(double cents)
{
    MicroPitchShifter shifter;
    shifter.setSampleRate(sampleRate);
    shifter.setCents(cents);

    std::vector<double> output;
    output.reserve(frameCount);
    for (int i = 0; i < frameCount; i++) {
        const double input = std::sin(2.0 * std::numbers::pi * inputHz * i / sampleRate);
        output.push_back(shifter.process(input));
    }
    return output;
}

double expectedHz(double cents)
{
    return inputHz * std::pow(2.0, cents / 1200.0);
}

} // namespace

void MicroPitchShifterTest::test_shift_up_shouldRaiseTheFrequency()
{
    const auto output = render(100.0);
    const double measured = measureFrequency(output, settleFrames);

    QVERIFY(measured > 0.0);
    QVERIFY(std::abs(measured - expectedHz(100.0)) / expectedHz(100.0) < 0.01);
}

void MicroPitchShifterTest::test_shift_down_shouldLowerTheFrequency()
{
    const auto output = render(-100.0);
    const double measured = measureFrequency(output, settleFrames);

    QVERIFY(measured > 0.0);
    QVERIFY(std::abs(measured - expectedHz(-100.0)) / expectedHz(-100.0) < 0.01);
}

void MicroPitchShifterTest::test_shift_zero_shouldLeaveTheFrequencyAlone()
{
    const auto output = render(0.0);
    const double measured = measureFrequency(output, settleFrames);

    QVERIFY(std::abs(measured - inputHz) / inputHz < 0.001);
}

void MicroPitchShifterTest::test_shift_anyAmount_shouldHoldItsLevel()
{
    // The crossfade is the whole difficulty of this technique: if the two gains do not square to
    // one, the output dips or bumps every time the read pointer wraps.
    for (const double cents : { 7.0, -7.0, 25.0, -25.0 }) {
        const auto output = render(cents);

        double peak = 0.0;
        double trough = 1.0;
        for (int i = settleFrames; i + 1024 < static_cast<int>(output.size()); i += 1024) {
            double windowPeak = 0.0;
            for (int k = 0; k < 1024; k++) {
                windowPeak = std::max(windowPeak, std::abs(output[static_cast<size_t>(i + k)]));
            }
            peak = std::max(peak, windowPeak);
            trough = std::min(trough, windowPeak);
        }

        QVERIFY2(trough > 0.9, qPrintable(QString { "%1 cents: trough %2" }.arg(cents).arg(trough)));
        QVERIFY2(peak < 1.1, qPrintable(QString { "%1 cents: peak %2" }.arg(cents).arg(peak)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MicroPitchShifterTest)
