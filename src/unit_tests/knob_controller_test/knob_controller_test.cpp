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

#include "knob_controller_test.hpp"
#include "../../common/constants.hpp"
#include "../../view/controllers/knob_controller.hpp"

#include <QTest>

#include <vector>

namespace noteahead {

void KnobControllerTest::test_intensityMapping_shouldMapValuesCorrectly()
{
    KnobController controller;
    const double from = 0.0;
    const double to = 1000.0;
    const double center = 500.0;

    // Test unmap (from internal value to normalized 0..1)
    QCOMPARE(controller.unmap(center, "intensity", from, to), 0.5);
    QCOMPARE(controller.unmap(to, "intensity", from, to), 1.0);
    QCOMPARE(controller.unmap(from, "intensity", from, to), 0.0);

    // Test map (from slider -1..1 to internal value)
    QCOMPARE(controller.map(0.5, "intensity", from, to), center); // 0.5 slider maps to center
    QCOMPARE(controller.map(1.0, "intensity", from, to), to);
    QCOMPARE(controller.map(0.0, "intensity", from, to), from);
}

void KnobControllerTest::test_intensityToString_shouldFormatPercentageStrings()
{
    KnobController controller;
    const double from = -100.0;
    const double to = 100.0;

    QCOMPARE(controller.bipolarToString(0.0, "%", from, to), QString { "0.0%" });
    QCOMPARE(controller.bipolarToString(100.0, "%", from, to), QString { "+100.0%" });
    QCOMPARE(controller.bipolarToString(-100.0, "%", from, to), QString { "-100.0%" });
    QCOMPARE(controller.bipolarToString(50.0, "%", from, to), QString { "+50.0%" });

    // Test visual snapping removal: 0.04% should now show (rounded) 0.0% but NOT be skipped by threshold
    QCOMPARE(controller.bipolarToString(0.04, "%", from, to), QString { "0.0%" });
}

void KnobControllerTest::test_panMapping_shouldMapValuesCorrectly()
{
    KnobController controller;
    const double from = 0.0;
    const double to = 1000.0;

    // Pan uses the same mapping as intensity
    QCOMPARE(controller.unmapPan(500.0, from, to), 0.0);
    QCOMPARE(controller.mapPan(1.0, from, to), 1000.0);
}

void KnobControllerTest::test_panToString_shouldFormatPanLabels()
{
    KnobController controller;
    const double from = 0.0;
    const double to = 1000.0;

    QCOMPARE(controller.panToString(500.0, from, to), QString { "Center" });
    QCOMPARE(controller.panToString(1000.0, from, to), QString { "+100.0% R" });
    QCOMPARE(controller.panToString(0.0, from, to), QString { "-100.0% L" });
}

void KnobControllerTest::test_timeMapping_shouldMapTimeValuesCorrectly()
{
    KnobController controller;
    const double from = 1.0;
    const double to = 10001.0;

    QCOMPARE(controller.unmapTime(from, from, to), 0.0);
    QCOMPARE(controller.unmapTime(to, from, to), 1.0);

    QCOMPARE(controller.mapTime(0.0, from, to), from);
    QCOMPARE(controller.mapTime(1.0, from, to), to);

    // Cubic mapping: 0.5^3 = 0.125. 1.0 + 0.125 * 10000 = 1251.0
    QCOMPARE(controller.mapTime(0.5, from, to), 1251.0);
}

void KnobControllerTest::test_timeToString_shouldFormatTimeStrings()
{
    KnobController controller;
    QCOMPARE(controller.timeToString(123.4, "ms"), QString { "123.4 ms" });
    QCOMPARE(controller.timeToString(10.0, "s"), QString { "10.0 s" });

    // New handling via ms suffix
    QCOMPARE(controller.timeToString(0.5, "ms"), QString { "0.5 ms" });
    QCOMPARE(controller.timeToString(0.005, "ms"), QString { "5 μs" });
    QCOMPARE(controller.timeToString(2000.0, "ms"), QString { "2.0 s" });
}

void KnobControllerTest::test_percentageToString_shouldFormatPercentageStrings()
{
    KnobController controller;
    QCOMPARE(controller.percentageToString(500.0), QString { "50.0%" });
    QCOMPARE(controller.percentageToString(1000.0), QString { "100.0%" });

    // Custom range
    QCOMPARE(controller.percentageToString(10.0, 0, 100), QString { "10.0%" });
}

void KnobControllerTest::test_decibelToString_shouldFormatDecibelStrings()
{
    KnobController controller;
    // Default 0..1000 -> -30..30
    QCOMPARE(controller.decibelToString(500.0), QString { "0.0 dB" });
    QCOMPARE(controller.decibelToString(1000.0), QString { "+30.0 dB" });
    QCOMPARE(controller.decibelToString(0.0), QString { "-30.0 dB" });

    // Custom range (e.g. Threshold -60..0)
    QCOMPARE(controller.decibelToString(-60.0, -60.0, 0.0), QString { "-60.0 dB" });
    QCOMPARE(controller.decibelToString(-20.0, -60.0, 0.0), QString { "-20.0 dB" });
    QCOMPARE(controller.decibelToString(0.0, -60.0, 0.0), QString { "0.0 dB" });

    // Custom range (e.g. Makeup -12..12)
    QCOMPARE(controller.decibelToString(-12.0, -12.0, 12.0), QString { "-12.0 dB" });
    QCOMPARE(controller.decibelToString(0.0, -12.0, 12.0), QString { "0.0 dB" });
    QCOMPARE(controller.decibelToString(6.0, -12.0, 12.0), QString { "+6.0 dB" });
}

void KnobControllerTest::test_decibelMultiplierToString_shouldFormatDecibelStrings()
{
    KnobController controller;
    QCOMPARE(controller.decibelMultiplierToString(1.0), QString { "0.0 dB" });
    QCOMPARE(controller.decibelMultiplierToString(2.0), QString { "+6.0 dB" });
    QCOMPARE(controller.decibelMultiplierToString(0.5), QString { "-6.0 dB" });
    QCOMPARE(controller.decibelMultiplierToString(0.0), QString { "-inf dB" });
}

void KnobControllerTest::test_valueToString_shouldFormatUnitStrings()
{
    KnobController controller;
    QCOMPARE(controller.valueToString(50.0, "%", 0, 100), QString { "50.0%" });
    QCOMPARE(controller.valueToString(-10.0, "dB", -60, 0), QString { "-10.0 dB" });
    QCOMPARE(controller.valueToString(123.0, "Hz"), QString { "123Hz" });

    // Test s and ms suffixes in valueToString (for linear mapping)
    QCOMPARE(controller.valueToString(500.0, "ms"), QString { "500.0 ms" });
    QCOMPARE(controller.valueToString(2.5, "s"), QString { "2.5 s" });
}

void KnobControllerTest::test_format_shouldHandleMappingAndUnits()
{
    KnobController controller;

    // Test exponential mapping with time suffixes (reproduce attack knob issue)
    // 0.5 normalized value should map to ~7.07 ms
    QCOMPARE(controller.format(0.5, "exponential", "ms", 0.1, 500.0), QString { "7.1 ms" });

    // Test logFrequency mapping (reproduce LPF Bypass issue)
    // Normalized 1.0 should show Bypass
    QCOMPARE(controller.format(1.0, "logFrequency", "%", 20, 20000), QString { "100.0% / Bypass" });

    // Test exponential mapping for Q (reproduce integer formatting issue)
    double normVal = controller.unmap(1.234, "exponential", 0.1, 10.0);
    QCOMPARE(controller.format(normVal, "exponential", "", 0.1, 10.0), QString { "1.23" });
}

void KnobControllerTest::test_parse_shouldReadTypedUnits()
{
    KnobController controller;

    // A threshold knob reads out in dB directly
    QCOMPARE(controller.parse("-3 dB", "linear", "dB", -60.0, 0.0).toDouble(), 57.0 / 60.0);
    QCOMPARE(controller.parse("-3", "linear", "dB", -60.0, 0.0).toDouble(), 57.0 / 60.0);

    // A decimal comma is what a Finnish keyboard offers on the numpad
    QCOMPARE(controller.parse("-6,0 dB", "linear", "dB", -60.0, 0.0).toDouble(), 54.0 / 60.0);

    // Time knobs scale themselves between μs, ms and s, so the typed unit has to be honoured
    QCOMPARE(controller.parse("250 ms", "linear", "ms", 0.0, 1000.0).toDouble(), 0.25);
    QCOMPARE(controller.parse("0.25 s", "linear", "ms", 0.0, 1000.0).toDouble(), 0.25);
    QCOMPARE(controller.parse("250", "linear", "ms", 0.0, 1000.0).toDouble(), 0.25);

    // Frequencies may be typed in kHz
    QCOMPARE(controller.parse("1.5 kHz", "logFrequency", "%", 20.0, 20000.0).toDouble(),
             controller.unmap(1500.0, "logFrequency", 20.0, 20000.0));
    QCOMPARE(controller.parse("Bypass", "logFrequency", "%", 20.0, 20000.0).toDouble(), 1.0);

    // Pan takes a side from either end of the input
    QCOMPARE(controller.parse("Center", "pan", "%", 0.0, 1.0).toDouble(), 0.5);
    QCOMPARE(controller.parse("L50", "pan", "%", 0.0, 1.0).toDouble(), 0.25);
    QCOMPARE(controller.parse("50.0% R", "pan", "%", 0.0, 1.0).toDouble(), 0.75);

    // A fader is typed in dB or in percent, and silence is a legal thing to ask for
    QCOMPARE(controller.parse("0 dB", "fader", "", 0.0, 1.0).toDouble(), controller.unmap(1.0, "fader", 0.0, 1.0));
    QCOMPARE(controller.parse("-inf dB", "fader", "", 0.0, 1.0).toDouble(), 0.0);

    // Out-of-range input lands on the end of the travel rather than off it
    QCOMPARE(controller.parse("+100 dB", "linear", "dB", -60.0, 0.0).toDouble(), 1.0);
    QCOMPARE(controller.parse("-999 dB", "linear", "dB", -60.0, 0.0).toDouble(), 0.0);
}

void KnobControllerTest::test_parse_shouldRoundTripFormattedStrings()
{
    KnobController controller;

    // Whatever a knob displays must read back as the position it was formatted from, so that the
    // string in the editor always means what it says. Composite readouts such as "33.3% / -9.5 dB"
    // are compared up to the slash: the trailing half is derived from a value the leading half has
    // already rounded, so it can land a tenth away and no parser can recover it.
    struct Case
    {
        QString type;
        QString suffix;
        double min = 0;
        double max = 1;
    };

    const std::vector<Case> cases = {
        { "linear", "dB", -60.0, 0.0 },
        { "linear", "%", 0.0, 1000.0 },
        { "linear", ":1", 1.0, 20.0 },
        { "exponential", "ms", 0.1, 500.0 },
        { "exponential", "", 0.1, 10.0 },
        { "cubic", "ms", 1.0, 10001.0 },
        { "logFrequency", "%", 20.0, 20000.0 },
        { "decibel", "", 0.0, 24.0 },
        { "bipolar", "%", -100.0, 100.0 },
        { "intensity", "%", 0.0, 1000.0 },
        { "integer", "", 1.0, 16.0 },
        { "pan", "%", 0.0, 1.0 },
        { "fader", "", 0.0, 1.0 },
        { "volume", "", 0.0, 1.0 }
    };

    for (auto && testCase : cases) {
        for (double position : { 0.0, 0.25, 0.5, 0.75, 1.0 }) {
            const QString formatted = controller.format(position, testCase.type, testCase.suffix, testCase.min, testCase.max);
            const QVariant parsed = controller.parse(formatted, testCase.type, testCase.suffix, testCase.min, testCase.max);
            QVERIFY2(parsed.isValid(), qPrintable(QString { "%1 '%2'" }.arg(testCase.type).arg(formatted)));
            const QString reformatted = controller.format(parsed.toDouble(), testCase.type, testCase.suffix, testCase.min, testCase.max);
            QCOMPARE(reformatted.section('/', 0, 0), formatted.section('/', 0, 0));
        }
    }
}

void KnobControllerTest::test_parse_shouldRejectTextWithoutANumber()
{
    KnobController controller;

    QVERIFY(!controller.parse("", "linear", "dB", -60.0, 0.0).isValid());
    QVERIFY(!controller.parse("nonsense", "linear", "dB", -60.0, 0.0).isValid());
    QVERIFY(!controller.parse("dB", "linear", "dB", -60.0, 0.0).isValid());
    QVERIFY(!controller.parse("inf dB", "fader", "", 0.0, 1.0).isValid());
}

void KnobControllerTest::test_bipolarMapping_shouldReadZeroAtTheCentre()
{
    KnobController controller;

    // A control centred on zero cannot use the plain percentage readout, which ignores the range
    // and reports the knob's own position: neutral would read 50 % and nothing would ever read
    // negative. That is what the Wave Designer's Attack and Sustain showed.
    QCOMPARE(controller.format(0.5, "bipolar", "%", -100.0, 100.0), QString { "0.0%" });
    QCOMPARE(controller.format(0.0, "bipolar", "%", -100.0, 100.0), QString { "-100.0%" });
    QCOMPARE(controller.format(1.0, "bipolar", "%", -100.0, 100.0), QString { "+100.0%" });
    QCOMPARE(controller.format(0.75, "bipolar", "%", -100.0, 100.0), QString { "+50.0%" });

    // Without a unit, as the exciter's Timbre wants it.
    QCOMPARE(controller.format(0.25, "bipolar", "", -50.0, 50.0), QString { "-25.0" });

    // The mapping itself stays linear, so the knob's travel is even.
    QCOMPARE(controller.map(0.5, "bipolar", -100.0, 100.0), 0.0);
    QCOMPARE(controller.map(0.25, "bipolar", -100.0, 100.0), -50.0);
}

void KnobControllerTest::test_frequencyToString_shouldFormatFrequencyStrings()
{
    KnobController controller;
    QCOMPARE(controller.frequencyToString(500.0, 440.0, false), QString { "440 Hz" });
    QCOMPARE(controller.frequencyToString(500.0, 1500.0, false), QString { "1.5 kHz" });
    QCOMPARE(controller.frequencyToString(1000.0, 20000.0, false), QString { "Bypass" });
    QCOMPARE(controller.frequencyToString(1000.0, 20000.0, true), QString { "20.0 kHz" });
    QCOMPARE(controller.frequencyToString(0.0, 0.0, false), QString { "0 Hz" });
}

void KnobControllerTest::test_syncLogic_shouldHandleSyncValuesAndLabels()
{
    KnobController controller;
    QCOMPARE(controller.syncCount(), 16);
    QCOMPARE(controller.syncLabel(0), QString { "1/1" });
    QCOMPARE(controller.syncLabel(15), QString { "1/64" });

    // 1/1 = 1.0. 1.0 * 1000 = 1000
    QCOMPARE(controller.syncValue(0), 1000.0);
    QCOMPARE(controller.syncIndex(1000.0), 0);

    // 1/2 = 0.5. 0.5 * 1000 = 500
    QCOMPARE(controller.syncIndex(500.0), 2);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::KnobControllerTest)
