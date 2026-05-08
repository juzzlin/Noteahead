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
#include "../../application/service/knob_controller.hpp"
#include "../../common/constants.hpp"

#include <QTest>

namespace noteahead {

void KnobControllerTest::test_intensityMapping()
{
    KnobController controller;
    const double from = 0.0;
    const double to = 1000.0;
    const double center = 500.0;

    // Test unmap (from internal value to slider -1..1)
    QCOMPARE(controller.unmapIntensity(center, from, to), 0.0);
    QCOMPARE(controller.unmapIntensity(to, from, to), 1.0);
    QCOMPARE(controller.unmapIntensity(from, from, to), -1.0);

    // Test map (from slider -1..1 to internal value)
    QCOMPARE(controller.mapIntensity(0.0, from, to), center);
    QCOMPARE(controller.mapIntensity(1.0, from, to), to);
    QCOMPARE(controller.mapIntensity(-1.0, from, to), from);

    // Test snap to center
    QCOMPARE(controller.mapIntensity(0.005, from, to), center);
    QCOMPARE(controller.mapIntensity(-0.005, from, to), center);
}

void KnobControllerTest::test_intensityToString()
{
    KnobController controller;
    const double from = 0.0;
    const double to = 1000.0;

    QCOMPARE(controller.intensityToString(500.0, from, to), QString("0.0%"));
    QCOMPARE(controller.intensityToString(1000.0, from, to), QString("+100.0%"));
    QCOMPARE(controller.intensityToString(0.0, from, to), QString("-100.0%"));
    QCOMPARE(controller.intensityToString(750.0, from, to), QString("+50.0%"));
}

void KnobControllerTest::test_panMapping()
{
    KnobController controller;
    const double from = 0.0;
    const double to = 1000.0;

    // Pan uses the same mapping as intensity
    QCOMPARE(controller.unmapPan(500.0, from, to), 0.0);
    QCOMPARE(controller.mapPan(1.0, from, to), 1000.0);
}

void KnobControllerTest::test_panToString()
{
    KnobController controller;
    const double from = 0.0;
    const double to = 1000.0;

    QCOMPARE(controller.panToString(500.0, from, to), QString("Center"));
    QCOMPARE(controller.panToString(1000.0, from, to), QString("+100.0% R"));
    QCOMPARE(controller.panToString(0.0, from, to), QString("-100.0% L"));
}

void KnobControllerTest::test_timeMapping()
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

void KnobControllerTest::test_timeToString()
{
    KnobController controller;
    QCOMPARE(controller.timeToString(123.4, "ms"), QString("123ms"));
    QCOMPARE(controller.timeToString(10.0, "s"), QString("10s"));
}

void KnobControllerTest::test_percentageToString()
{
    KnobController controller;
    QCOMPARE(controller.percentageToString(500.0), QString("50.0%"));
    QCOMPARE(controller.percentageToString(1000.0), QString("100.0%"));
}

void KnobControllerTest::test_decibelToString()
{
    KnobController controller;
    // 0..1000 -> -30..30
    QCOMPARE(controller.decibelToString(500.0), QString("0.0 dB"));
    QCOMPARE(controller.decibelToString(1000.0), QString("+30.0 dB"));
    QCOMPARE(controller.decibelToString(0.0), QString("-30.0 dB"));
}

void KnobControllerTest::test_frequencyToString()
{
    KnobController controller;
    QCOMPARE(controller.frequencyToString(500.0, 440.0, false), QString("440 Hz"));
    QCOMPARE(controller.frequencyToString(500.0, 1500.0, false), QString("1.5 kHz"));
    QCOMPARE(controller.frequencyToString(1000.0, 20000.0, false), QString("Bypass"));
    QCOMPARE(controller.frequencyToString(1000.0, 20000.0, true), QString("20.0 kHz"));
    QCOMPARE(controller.frequencyToString(0.0, 0.0, false), QString("0 Hz"));
}

void KnobControllerTest::test_syncLogic()
{
    KnobController controller;
    QCOMPARE(controller.syncCount(), 16);
    QCOMPARE(controller.syncLabel(0), QString("1/1"));
    QCOMPARE(controller.syncLabel(15), QString("1/64"));

    // 1/1 = 1.0. 1.0 * 1000 = 1000
    QCOMPARE(controller.syncValue(0), 1000.0);
    QCOMPARE(controller.syncIndex(1000.0), 0);
    
    // 1/2 = 0.5. 0.5 * 1000 = 500
    QCOMPARE(controller.syncIndex(500.0), 2);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::KnobControllerTest)
