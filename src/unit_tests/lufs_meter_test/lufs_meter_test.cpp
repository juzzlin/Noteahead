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

#include "lufs_meter_test.hpp"

#include "../../domain/dsp/lufs_meter.hpp"

#include <QTest>

#include <cmath>
#include <numbers>

namespace noteahead {

static constexpr double sampleRate = 48000.0;
static constexpr float lufsFloor = -70.0f;

// Feed silence for the given number of seconds and fill the ring buffer.
static void feedSilence(LufsMeter & meter, double seconds)
{
    const int samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        double l = 0.0, r = 0.0;
        meter.process(l, r);
    }
}

// Feed a stereo sine for the given number of seconds.
static void feedSine(LufsMeter & meter, double freq, double amplitude, double seconds)
{
    const int samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        const double s = amplitude * std::sin(2.0 * std::numbers::pi * freq / sampleRate * i);
        double l = s, r = s;
        meter.process(l, r);
    }
}

void LufsMeterTest::test_lufsMeter_silence_shouldReturnFloor()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    // Before any audio: both readings are at the floor
    QCOMPARE(meter.momentaryLufs(), lufsFloor);
    QCOMPARE(meter.shortTermLufs(), lufsFloor);

    feedSilence(meter, 3.0);

    QCOMPARE(meter.momentaryLufs(), lufsFloor);
    QCOMPARE(meter.shortTermLufs(), lufsFloor);
}

void LufsMeterTest::test_lufsMeter_sine_shouldMeasureLoudness()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    // Amplitude 0.1 at 1kHz. K-weighting barely changes 1kHz so expected loudness:
    // L ≈ -0.691 + 10*log10(2 * 0.01/2) = -0.691 + 10*log10(0.01) ≈ -20.7 LUFS
    feedSine(meter, 1000.0, 0.1, 3.5);

    // Allow ±3 dB tolerance for K-weighting and block-boundary effects
    QVERIFY(meter.shortTermLufs() > -24.0f);
    QVERIFY(meter.shortTermLufs() < -18.0f);

    // Momentary is also active
    QVERIFY(meter.momentaryLufs() > -24.0f);
    QVERIFY(meter.momentaryLufs() < -18.0f);
}

void LufsMeterTest::test_lufsMeter_passThrough_shouldNotModifySignal()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    // The meter must not alter the audio stream
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

void LufsMeterTest::test_lufsMeter_reset_shouldClearReadings()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    feedSine(meter, 1000.0, 0.5, 3.5);

    // Confirm something is being measured before reset
    QVERIFY(meter.shortTermLufs() > lufsFloor);

    meter.reset();

    QCOMPARE(meter.momentaryLufs(), lufsFloor);
    QCOMPARE(meter.shortTermLufs(), lufsFloor);
}

void LufsMeterTest::test_lufsMeter_sampleRateChange_shouldReinitialize()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    feedSine(meter, 1000.0, 0.5, 3.5);
    QVERIFY(meter.shortTermLufs() > lufsFloor);

    // Changing sample rate resets state and updates block size
    meter.setSampleRate(44100.0);
    feedSine(meter, 1000.0, 0.001, 1.0);

    // Should now be measuring the quiet signal, not the old loud one
    QVERIFY(meter.momentaryLufs() < -30.0f);
}

} // namespace noteahead

QTEST_MAIN(noteahead::LufsMeterTest)
