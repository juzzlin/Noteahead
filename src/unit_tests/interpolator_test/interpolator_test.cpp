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

#include "interpolator_test.hpp"

#include "../../domain/tracker/interpolator.hpp"

#include <QTest>

#include <vector>

namespace noteahead {

using CurveType = Interpolator::CurveType;

static const std::vector<CurveType> allCurves = {
    CurveType::Linear,
    CurveType::Exponential,
    CurveType::Logarithmic,
    CurveType::EaseIn,
    CurveType::EaseOut,
    CurveType::EaseInOut
};

void InterpolatorTest::test_interpolator_defaultCurve_shouldBeLinear()
{
    const Interpolator interpolator { 0, 8, 0, 80 };

    QCOMPARE(interpolator.curve(), CurveType::Linear);

    // The pre-curve behaviour of existing projects must be preserved exactly
    for (size_t line = 0; line <= 8; line++) {
        QCOMPARE(interpolator.getValue(line), static_cast<double>(line) * 10);
    }
}

void InterpolatorTest::test_interpolator_linear_shouldRampEvenly()
{
    const Interpolator interpolator { 0, 10, 0, 100, CurveType::Linear };

    QCOMPARE(interpolator.getValue(2), 20.0);
    QCOMPARE(interpolator.getValue(5), 50.0);
    QCOMPARE(interpolator.getValue(8), 80.0);
}

void InterpolatorTest::test_interpolator_allCurves_shouldHitEndPoints()
{
    for (auto && curve : allCurves) {
        const Interpolator interpolator { 4, 12, 20, 90, curve };
        QCOMPARE(interpolator.getValue(4), 20.0);
        QCOMPARE(interpolator.getValue(12), 90.0);
    }
}

void InterpolatorTest::test_interpolator_allCurves_shouldBeMonotonic()
{
    for (auto && curve : allCurves) {
        const Interpolator interpolator { 0, 32, 0, 127, curve };
        double previous = interpolator.getValue(0);
        for (size_t line = 1; line <= 32; line++) {
            const double current = interpolator.getValue(line);
            QVERIFY(current >= previous);
            previous = current;
        }
    }
}

void InterpolatorTest::test_interpolator_allCurves_shouldStayWithinRange()
{
    for (auto && curve : allCurves) {
        const Interpolator interpolator { 0, 16, 30, 100, curve };
        for (size_t line = 0; line <= 16; line++) {
            const double value = interpolator.getValue(line);
            QVERIFY(value >= 30.0);
            QVERIFY(value <= 100.0);
        }
    }
}

void InterpolatorTest::test_interpolator_exponential_shouldStartSlowly()
{
    const Interpolator exponential { 0, 10, 0, 100, CurveType::Exponential };
    const Interpolator linear { 0, 10, 0, 100, CurveType::Linear };

    // Slow start, fast end: below the line everywhere in between
    for (size_t line = 1; line < 10; line++) {
        QVERIFY(exponential.getValue(line) < linear.getValue(line));
    }
    QCOMPARE(exponential.getValue(5), 25.0);
}

void InterpolatorTest::test_interpolator_logarithmic_shouldStartQuickly()
{
    const Interpolator logarithmic { 0, 10, 0, 100, CurveType::Logarithmic };
    const Interpolator linear { 0, 10, 0, 100, CurveType::Linear };

    // Fast start, slow end: above the line everywhere in between
    for (size_t line = 1; line < 10; line++) {
        QVERIFY(logarithmic.getValue(line) > linear.getValue(line));
    }
    QCOMPARE(logarithmic.getValue(5), 75.0);
}

void InterpolatorTest::test_interpolator_easeIn_shouldStartSlowerThanExponential()
{
    const Interpolator easeIn { 0, 10, 0, 100, CurveType::EaseIn };
    const Interpolator exponential { 0, 10, 0, 100, CurveType::Exponential };

    for (size_t line = 1; line < 10; line++) {
        QVERIFY(easeIn.getValue(line) < exponential.getValue(line));
    }
    QCOMPARE(easeIn.getValue(5), 12.5);
}

void InterpolatorTest::test_interpolator_easeOut_shouldMirrorEaseIn()
{
    const Interpolator easeIn { 0, 10, 0, 100, CurveType::EaseIn };
    const Interpolator easeOut { 0, 10, 0, 100, CurveType::EaseOut };

    for (size_t line = 0; line <= 10; line++) {
        QCOMPARE(easeOut.getValue(line), 100.0 - easeIn.getValue(10 - line));
    }
}

void InterpolatorTest::test_interpolator_easeInOut_shouldBeSymmetricAroundMidPoint()
{
    const Interpolator easeInOut { 0, 10, 0, 100, CurveType::EaseInOut };

    QCOMPARE(easeInOut.getValue(5), 50.0);
    for (size_t line = 0; line <= 10; line++) {
        QCOMPARE(easeInOut.getValue(line), 100.0 - easeInOut.getValue(10 - line));
    }
    // Slow at both ends, steep in the middle
    QVERIFY(easeInOut.getValue(1) < 10.0);
    QVERIFY(easeInOut.getValue(9) > 90.0);
}

void InterpolatorTest::test_interpolator_curves_shouldSupportDescendingRamps()
{
    for (auto && curve : allCurves) {
        const Interpolator interpolator { 0, 10, 100, 0, curve };
        QCOMPARE(interpolator.getValue(0), 100.0);
        QCOMPARE(interpolator.getValue(10), 0.0);
        double previous = interpolator.getValue(0);
        for (size_t line = 1; line <= 10; line++) {
            const double current = interpolator.getValue(line);
            QVERIFY(current <= previous);
            previous = current;
        }
    }
}

void InterpolatorTest::test_interpolator_curves_shouldClampOutsideLineRange()
{
    for (auto && curve : allCurves) {
        const Interpolator interpolator { 5, 10, 20, 60, curve };
        QCOMPARE(interpolator.getValue(0), 20.0);
        QCOMPARE(interpolator.getValue(4), 20.0);
        QCOMPARE(interpolator.getValue(11), 60.0);
        QCOMPARE(interpolator.getValue(1000), 60.0);
    }
}

void InterpolatorTest::test_interpolator_zeroLengthRange_shouldReturnStartValue()
{
    for (auto && curve : allCurves) {
        const Interpolator interpolator { 7, 7, 42, 99, curve };
        QCOMPARE(interpolator.getValue(7), 42.0);
    }
}

void InterpolatorTest::test_interpolator_xmlValues_shouldRoundTrip()
{
    for (auto && curve : allCurves) {
        QCOMPARE(Interpolator::curveFromXmlValue(Interpolator::curveToXmlValue(curve)), curve);
    }
}

void InterpolatorTest::test_interpolator_xmlValue_unknownName_shouldFallBackToLinear()
{
    // Projects saved before curves existed have no curve attribute at all
    QCOMPARE(Interpolator::curveFromXmlValue({}), CurveType::Linear);
    QCOMPARE(Interpolator::curveFromXmlValue("NoSuchCurve"), CurveType::Linear);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::InterpolatorTest)
