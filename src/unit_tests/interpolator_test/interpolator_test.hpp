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

#ifndef INTERPOLATOR_TEST_HPP
#define INTERPOLATOR_TEST_HPP

#include <QObject>

namespace noteahead {

class InterpolatorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_interpolator_defaultCurve_shouldBeLinear();
    void test_interpolator_linear_shouldRampEvenly();
    void test_interpolator_allCurves_shouldHitEndPoints();
    void test_interpolator_allCurves_shouldBeMonotonic();
    void test_interpolator_allCurves_shouldStayWithinRange();
    void test_interpolator_exponential_shouldStartSlowly();
    void test_interpolator_logarithmic_shouldStartQuickly();
    void test_interpolator_easeIn_shouldStartSlowerThanExponential();
    void test_interpolator_easeOut_shouldMirrorEaseIn();
    void test_interpolator_easeInOut_shouldBeSymmetricAroundMidPoint();
    void test_interpolator_curves_shouldSupportDescendingRamps();
    void test_interpolator_curves_shouldClampOutsideLineRange();
    void test_interpolator_zeroLengthRange_shouldReturnStartValue();
    void test_interpolator_xmlValues_shouldRoundTrip();
    void test_interpolator_xmlValue_unknownName_shouldFallBackToLinear();
};

} // namespace noteahead

#endif // INTERPOLATOR_TEST_HPP
