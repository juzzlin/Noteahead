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

#include "dbtp_meter_test.hpp"

#include "../../domain/dsp/dbtp_meter.hpp"

#include <QTest>

#include <cmath>
#include <numbers>

namespace noteahead {

static constexpr double sampleRate = 48000.0;
static constexpr float meterFloor = -70.0f;

static void feedSilence(DbTpMeter & meter, double seconds)
{
    const int samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        double l = 0.0, r = 0.0;
        meter.process(l, r);
    }
}

static void feedSine(DbTpMeter & meter, double freq, double amplitude, double seconds)
{
    const int samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        const double s = amplitude * std::sin(2.0 * std::numbers::pi * freq / sampleRate * i);
        double l = s, r = s;
        meter.process(l, r);
    }
}

void DbTpMeterTest::test_dbtpMeter_silence_shouldReturnFloor()
{
    DbTpMeter meter;
    meter.setSampleRate(sampleRate);

    QCOMPARE(meter.truePeakL(), meterFloor);
    QCOMPARE(meter.truePeakR(), meterFloor);
    QCOMPARE(meter.truePeakHoldL(), meterFloor);
    QCOMPARE(meter.truePeakHoldR(), meterFloor);

    feedSilence(meter, 1.0);

    QCOMPARE(meter.truePeakL(), meterFloor);
    QCOMPARE(meter.truePeakR(), meterFloor);
}

void DbTpMeterTest::test_dbtpMeter_fullScale_shouldReturnNearZeroDbtp()
{
    DbTpMeter meter;
    meter.setSampleRate(sampleRate);

    // Amplitude 1.0 sine at 1kHz: true peak should be near 0 dBTP
    feedSine(meter, 1000.0, 1.0, 0.5);

    // Allow some tolerance — Hermite interpolation may push slightly over 0 dBTP for a sine
    QVERIFY(meter.truePeakHoldL() > -3.0f);
    QVERIFY(meter.truePeakHoldL() <= 3.0f);
    QVERIFY(meter.truePeakHoldR() > -3.0f);
    QVERIFY(meter.truePeakHoldR() <= 3.0f);
}

void DbTpMeterTest::test_dbtpMeter_passThrough_shouldNotModifySignal()
{
    DbTpMeter meter;
    meter.setSampleRate(sampleRate);

    for (int i = 0; i < 1000; i++) {
        const double origL = std::sin(2.0 * std::numbers::pi * 440.0 / sampleRate * i);
        const double origR = std::cos(2.0 * std::numbers::pi * 440.0 / sampleRate * i);
        double l = origL;
        double r = origR;
        meter.process(l, r);
        QCOMPARE(l, origL);
        QCOMPARE(r, origR);
    }
}

void DbTpMeterTest::test_dbtpMeter_reset_shouldClearReadings()
{
    DbTpMeter meter;
    meter.setSampleRate(sampleRate);

    feedSine(meter, 1000.0, 0.5, 1.0);
    QVERIFY(meter.truePeakHoldL() > meterFloor);

    meter.reset();

    QCOMPARE(meter.truePeakL(), meterFloor);
    QCOMPARE(meter.truePeakR(), meterFloor);
    QCOMPARE(meter.truePeakHoldL(), meterFloor);
    QCOMPARE(meter.truePeakHoldR(), meterFloor);
}

void DbTpMeterTest::test_dbtpMeter_sampleRateChange_shouldReinitialize()
{
    DbTpMeter meter;
    meter.setSampleRate(sampleRate);

    feedSine(meter, 1000.0, 0.5, 1.0);
    QVERIFY(meter.truePeakHoldL() > meterFloor);

    meter.setSampleRate(44100.0);
    // Reinitializes — state is cleared after sample-rate change triggers reset in process()
    feedSilence(meter, 0.1);

    QCOMPARE(meter.truePeakHoldL(), meterFloor);
}

void DbTpMeterTest::test_dbtpMeter_peakHold_shouldRetainMaximum()
{
    DbTpMeter meter;
    meter.setSampleRate(sampleRate);

    // Loud burst, then silence
    feedSine(meter, 1000.0, 0.5, 0.1);
    const float peakAfterBurst = meter.truePeakHoldL();
    QVERIFY(peakAfterBurst > meterFloor);

    // Short silence — hold counter still active, peak hold must not drop
    feedSilence(meter, 0.5);
    QVERIFY(meter.truePeakHoldL() >= peakAfterBurst - 1.0f);
}

} // namespace noteahead

QTEST_MAIN(noteahead::DbTpMeterTest)
