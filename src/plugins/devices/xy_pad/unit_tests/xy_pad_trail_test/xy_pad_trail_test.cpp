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

#include "xy_pad_trail_test.hpp"

#include "../../xy_pad_trail.hpp"

#include <QTest>

namespace noteahead {

namespace {

using namespace std::chrono_literals;

const auto start = XyPadTrail::Clock::time_point {} + 1h;

} // namespace

void XyPadTrailTest::test_points_moving_shouldAgeFromOldestToNewest()
{
    XyPadTrail trail;
    trail.add({ 0.0, 0.0 }, start);
    trail.add({ 0.1, 0.0 }, start + 100ms);
    trail.add({ 0.2, 0.0 }, start + 200ms);

    const auto points = trail.points(start + 200ms);
    QCOMPARE(points.size(), size_t { 3 });
    QCOMPARE(points.back().position, QPointF(0.2, 0.0));
    QCOMPARE(points.back().age, 0.0);
    QVERIFY(points.front().age > points.at(1).age);
    QVERIFY(std::abs(points.front().age - 200.0 / 600.0) < 1.0e-9);
    QVERIFY(!trail.isEmpty(start + 200ms));
}

void XyPadTrailTest::test_points_pastLifetime_shouldExpire()
{
    XyPadTrail trail;
    trail.add({ 0.0, 0.0 }, start);
    trail.add({ 0.5, 0.5 }, start + 100ms);

    QVERIFY(!trail.isEmpty(start + 100ms + XyPadTrail::Lifetime / 2));
    QCOMPARE(trail.points(start + XyPadTrail::Lifetime + 50ms).size(), size_t { 1 });
    QVERIFY(trail.isEmpty(start + XyPadTrail::Lifetime + 100ms));
}

void XyPadTrailTest::test_add_samePosition_shouldAddNothing()
{
    XyPadTrail trail;
    trail.add({ 0.3, 0.3 }, start);
    trail.add({ 0.3, 0.3 }, start + 16ms);
    trail.add({ 0.3, 0.3 }, start + 32ms);

    // A pad at rest leaves no tail, and what it had fades away
    QCOMPARE(trail.points(start + 32ms).size(), size_t { 1 });
    QVERIFY(trail.isEmpty(start + 32ms));
}

void XyPadTrailTest::test_add_afterStandingStill_shouldStartFromTheRestingPoint()
{
    XyPadTrail trail;
    trail.add({ 0.0, 0.0 }, start);
    trail.add({ 0.5, 0.0 }, start + 2s);

    // The move draws from where the pad stood, not from nowhere
    const auto points = trail.points(start + 2s);
    QCOMPARE(points.size(), size_t { 2 });
    QCOMPARE(points.front().position, QPointF(0.0, 0.0));
    QVERIFY(!trail.isEmpty(start + 2s));
}

void XyPadTrailTest::test_add_manyPoints_shouldKeepTheNewest()
{
    XyPadTrail trail;
    const size_t count = XyPadTrail::MaxPoints * 2;
    for (size_t index = 0; index < count; index++) {
        trail.add({ static_cast<double>(index) / static_cast<double>(count), 0.0 }, start + std::chrono::milliseconds { index });
    }

    const auto points = trail.points(start + std::chrono::milliseconds { count });
    QCOMPARE(points.size(), XyPadTrail::MaxPoints);
    QCOMPARE(points.back().position.x(), static_cast<double>(count - 1) / static_cast<double>(count));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XyPadTrailTest)
