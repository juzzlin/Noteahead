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

#include "level_meter_test.hpp"

#include "../../domain/utility/level_meter.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr uint32_t FrameCount { 512 };

std::vector<double> constantBuffer(double amplitude)
{
    return std::vector<double>(static_cast<size_t>(FrameCount) * 2, amplitude);
}

std::vector<double> sineBuffer(double amplitude, double frequency)
{
    std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, 0.0);
    for (uint32_t frame = 0; frame < FrameCount; frame++) {
        const double value = amplitude * std::sin(2.0 * std::numbers::pi * frequency * frame / SampleRate);
        samples[frame * 2] = value;
        samples[frame * 2 + 1] = value;
    }
    return samples;
}

//! Feeds the same buffer repeatedly so the smoothed RMS reading settles.
void feed(LevelMeter & meter, const std::vector<double> & samples, int buffers)
{
    for (int i = 0; i < buffers; i++) {
        meter.write(samples.data(), FrameCount, SampleRate);
    }
}

} // namespace

void LevelMeterTest::test_write_inactive_shouldStaySilent()
{
    // A meter nobody is looking at must cost nothing and report nothing.
    LevelMeter meter;
    feed(meter, constantBuffer(1.0), 10);

    QCOMPARE(meter.peakDb(), LevelMeter::MinimumDb);
    QCOMPARE(meter.rmsDb(), LevelMeter::MinimumDb);
}

void LevelMeterTest::test_write_fullScale_shouldReadZeroDbfs()
{
    LevelMeter meter;
    meter.setActive(true);
    feed(meter, constantBuffer(1.0), 200);

    QVERIFY(std::abs(meter.peakDb()) < 0.01f);
    QVERIFY2(std::abs(meter.rmsDb()) < 0.1f, qPrintable(QString::number(meter.rmsDb())));
}

void LevelMeterTest::test_write_minusEighteen_shouldReadTheTarget()
{
    // The gain staging target the meter marks: a device trimmed to -18 dBFS must read -18.
    const double amplitude = std::pow(10.0, -18.0 / 20.0);

    LevelMeter meter;
    meter.setActive(true);
    feed(meter, constantBuffer(amplitude), 200);

    QVERIFY2(std::abs(meter.peakDb() + 18.0f) < 0.01f, qPrintable(QString::number(meter.peakDb())));
    QVERIFY2(std::abs(meter.rmsDb() + 18.0f) < 0.1f, qPrintable(QString::number(meter.rmsDb())));
}

void LevelMeterTest::test_write_sine_shouldSeparatePeakFromRms()
{
    // A sine's RMS sits 3.01 dB below its peak, which is what makes the two readings worth showing.
    LevelMeter meter;
    meter.setActive(true);
    feed(meter, sineBuffer(1.0, 1000.0), 200);

    QVERIFY2(std::abs(meter.peakDb()) < 0.1f, qPrintable(QString::number(meter.peakDb())));
    QVERIFY2(std::abs(meter.rmsDb() + 3.01f) < 0.2f, qPrintable(QString::number(meter.rmsDb())));
}

void LevelMeterTest::test_peak_shouldFallBackWhenSignalStops()
{
    LevelMeter meter;
    meter.setActive(true);
    feed(meter, constantBuffer(1.0), 20);
    const float loud = meter.peakDb();

    // One second of silence at the specified 20 dB/s fallback.
    feed(meter, constantBuffer(0.0), static_cast<int>(SampleRate / FrameCount));
    const float quiet = meter.peakDb();

    QVERIFY(std::abs(loud) < 0.01f);
    QVERIFY2(quiet < loud - 15.0f, qPrintable(QString { "%1 -> %2" }.arg(loud).arg(quiet)));
    QVERIFY(quiet > loud - 25.0f);
}

void LevelMeterTest::test_reset_shouldClearLevels()
{
    LevelMeter meter;
    meter.setActive(true);
    feed(meter, constantBuffer(1.0), 20);
    meter.reset();

    QCOMPARE(meter.peakDb(), LevelMeter::MinimumDb);
    QCOMPARE(meter.rmsDb(), LevelMeter::MinimumDb);
}

void LevelMeterTest::test_setActive_false_shouldClearLevels()
{
    // Otherwise a meter would come back showing whatever was playing when it was last switched off.
    LevelMeter meter;
    meter.setActive(true);
    feed(meter, constantBuffer(1.0), 20);
    meter.setActive(false);

    QCOMPARE(meter.peakDb(), LevelMeter::MinimumDb);
    QVERIFY(!meter.active());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LevelMeterTest)
