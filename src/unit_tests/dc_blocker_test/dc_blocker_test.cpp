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

#include "dc_blocker_test.hpp"

#include "../../domain/dsp/dc_blocker.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

// After 10 time constants the residual is < e^{-10} ≈ 0.000045.
// Time constant = 1/(1 - R) ≈ 1528 samples at 48 kHz with R = exp(-2π*5/48000).
static constexpr int settlesamples = 20000;

void DcBlockerTest::test_dcBlocker_constantInput_shouldConvergeToZero()
{
    DcBlocker blocker;
    blocker.setSampleRate(48000.0);

    double output = 0.0;
    for (int i = 0; i < settlesamples; i++) {
        output = blocker.process(1.0);
    }

    QVERIFY(std::abs(output) < 1e-3);
}

void DcBlockerTest::test_dcBlocker_dcPlusSine_shouldRemoveDcFromOutput()
{
    DcBlocker blocker;
    blocker.setSampleRate(48000.0);

    constexpr double dc = 0.5;
    constexpr double freq = 440.0;
    constexpr double sr = 48000.0;
    constexpr int measureSamples = 4800;

    double meanOut = 0.0;
    for (int i = 0; i < settlesamples + measureSamples; i++) {
        const double input = dc + std::sin(2.0 * M_PI * freq / sr * i);
        const double out = blocker.process(input);
        if (i >= settlesamples) {
            meanOut += out;
        }
    }
    meanOut /= measureSamples;

    QVERIFY(std::abs(meanOut) < 1e-3);
}

void DcBlockerTest::test_dcBlocker_highFrequencySine_shouldPassThrough()
{
    DcBlocker blocker;
    blocker.setSampleRate(48000.0);

    constexpr double freq = 1000.0;
    constexpr double sr = 48000.0;
    constexpr int measureSamples = 4800;

    double sumSqIn = 0.0;
    double sumSqOut = 0.0;
    for (int i = 0; i < settlesamples + measureSamples; i++) {
        const double input = std::sin(2.0 * M_PI * freq / sr * i);
        const double out = blocker.process(input);
        if (i >= settlesamples) {
            sumSqIn += input * input;
            sumSqOut += out * out;
        }
    }

    const double rmsIn = std::sqrt(sumSqIn / measureSamples);
    const double rmsOut = std::sqrt(sumSqOut / measureSamples);

    QVERIFY(std::abs(rmsOut - rmsIn) < 1e-3);
}

void DcBlockerTest::test_dcBlocker_reset_shouldClearState()
{
    DcBlocker blocker;
    blocker.setSampleRate(48000.0);

    for (int i = 0; i < 1000; i++) {
        blocker.process(1.0);
    }

    blocker.reset();

    // After reset an impulse should produce a bounded, finite output
    const double out = blocker.process(1.0);
    QVERIFY(std::isfinite(out));
    QVERIFY(std::abs(out) <= 2.0);
}

} // namespace noteahead

QTEST_MAIN(noteahead::DcBlockerTest)
