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

#include "delay_line_test.hpp"

#include "../../domain/dsp/delay_line.hpp"

#include <QTest>

namespace noteahead {

void DelayLineTest::test_read_shouldReturnZero_whenBufferEmpty()
{
    DelayLine dl;
    QCOMPARE(dl.read(), 0.0);
}

void DelayLineTest::test_read_shouldReturnZero_beforeDelayElapsed()
{
    DelayLine dl;
    dl.setMaxDelay(4);
    dl.setDelay(4);

    // Write fewer steps than the delay — output must stay zero
    dl.write(1.0);
    dl.write(1.0);
    dl.write(1.0);
    QCOMPARE(dl.read(), 0.0);
}

void DelayLineTest::test_read_shouldReturnWrittenValue_afterDelaySteps()
{
    DelayLine dl;
    constexpr size_t D = 4;
    dl.setMaxDelay(D);
    dl.setDelay(D);

    // Write a known value followed by zeros
    dl.write(1.0);
    for (size_t i = 1; i < D; i++) {
        dl.write(0.0);
    }

    // After D writes the initial value should be readable
    QCOMPARE(dl.read(), 1.0);
}

void DelayLineTest::test_delay_shouldMatchConfiguredLength()
{
    DelayLine dl;
    constexpr size_t D = 8;
    dl.setMaxDelay(D);
    dl.setDelay(D);

    // Fill with a unique sequence
    for (size_t i = 0; i < D; i++) {
        dl.write(static_cast<double>(i + 1));
    }

    // Each further write/read cycle should yield the corresponding old value
    for (size_t i = 0; i < D; i++) {
        const double expected = static_cast<double>(i + 1);
        QCOMPARE(dl.read(), expected);
        dl.write(0.0);
    }
}

void DelayLineTest::test_reset_shouldClearBuffer()
{
    DelayLine dl;
    constexpr size_t D = 4;
    dl.setMaxDelay(D);
    dl.setDelay(D);

    for (size_t i = 0; i < D; i++) {
        dl.write(1.0);
    }

    dl.reset();

    // After reset every read should return zero regardless of how many writes follow
    for (size_t i = 0; i < D; i++) {
        QCOMPARE(dl.read(), 0.0);
        dl.write(0.0);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DelayLineTest)
