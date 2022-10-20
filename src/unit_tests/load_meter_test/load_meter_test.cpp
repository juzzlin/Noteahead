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

#include "load_meter_test.hpp"

#include "../../domain/utility/load_meter.hpp"

#include <QTest>

#include <chrono>
#include <cmath>

namespace noteahead {

namespace {

using namespace std::chrono_literals;

//! One 256-frame buffer at 48 kHz, the realistic case.
constexpr double BufferSeconds { 256.0 / 48000.0 };

void feed(LoadMeter & meter, double loadPercent, int blocks)
{
    const auto elapsed = std::chrono::nanoseconds { static_cast<int64_t>(BufferSeconds * loadPercent / 100.0 * 1.0e9) };
    for (int i = 0; i < blocks; i++) {
        meter.addBlock(elapsed, BufferSeconds);
    }
}

} // namespace

void LoadMeterTest::test_addBlock_inactive_shouldStaySilent()
{
    LoadMeter meter;
    feed(meter, 50.0, 200);

    QCOMPARE(meter.loadPercent(), 0.0f);
    QCOMPARE(meter.overrunCount(), uint64_t { 0 });
}

void LoadMeterTest::test_addBlock_halfBudget_shouldReadFiftyPercent()
{
    // Taking half as long to render a buffer as the buffer lasts is 50% load, by definition.
    LoadMeter meter;
    meter.setActive(true);
    feed(meter, 50.0, 500);

    QVERIFY2(std::abs(meter.loadPercent() - 50.0f) < 1.0f, qPrintable(QString::number(meter.loadPercent())));
}

void LoadMeterTest::test_addBlock_shouldSettleOnTheAverage()
{
    LoadMeter meter;
    meter.setActive(true);
    feed(meter, 20.0, 500);
    QVERIFY(std::abs(meter.loadPercent() - 20.0f) < 1.0f);

    // A sustained change must be followed, not averaged away forever.
    feed(meter, 80.0, 500);
    QVERIFY2(std::abs(meter.loadPercent() - 80.0f) < 1.0f, qPrintable(QString::number(meter.loadPercent())));
}

void LoadMeterTest::test_overrunCount_shouldCountOnlyBlocksOverBudget()
{
    LoadMeter meter;
    meter.setActive(true);

    feed(meter, 99.0, 100);
    QCOMPARE(meter.overrunCount(), uint64_t { 0 });

    feed(meter, 150.0, 7);
    QCOMPARE(meter.overrunCount(), uint64_t { 7 });
}

void LoadMeterTest::test_peak_shouldFallBackAfterASpike()
{
    LoadMeter meter;
    meter.setActive(true);
    feed(meter, 10.0, 100);
    feed(meter, 90.0, 1);
    const float afterSpike = meter.peakPercent();

    // One second of quiet buffers at the specified fallback rate.
    feed(meter, 10.0, static_cast<int>(1.0 / BufferSeconds));

    QVERIFY2(afterSpike > 85.0f, qPrintable(QString::number(afterSpike)));
    QVERIFY2(meter.peakPercent() < afterSpike - 30.0f, qPrintable(QString::number(meter.peakPercent())));
}

void LoadMeterTest::test_addBlock_zeroWork_shouldDecayToIdle()
{
    // The engine skips a silent device entirely, and reports that as zero-cost blocks. Without
    // those the meter would keep showing whatever the device last cost, for as long as it stayed
    // silent, which reads as a device burning CPU while doing nothing.
    LoadMeter meter;
    meter.setActive(true);
    feed(meter, 40.0, 500);
    QVERIFY(meter.loadPercent() > 35.0f);

    feed(meter, 0.0, 500);

    QVERIFY2(meter.loadPercent() < 1.0f, qPrintable(QString::number(meter.loadPercent())));
}

void LoadMeterTest::test_setActive_false_shouldClearEverything()
{
    LoadMeter meter;
    meter.setActive(true);
    feed(meter, 150.0, 10);
    QVERIFY(meter.overrunCount() > 0);

    meter.setActive(false);

    QCOMPARE(meter.loadPercent(), 0.0f);
    QCOMPARE(meter.peakPercent(), 0.0f);
    QCOMPARE(meter.overrunCount(), uint64_t { 0 });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LoadMeterTest)
