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

#include "panner_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/effects/panner_effect.hpp"

#include <QTest>
#include <cmath>
#include <numbers>

namespace noteahead {

void PannerTest::test_pan_shouldAdjustGains()
{
    PannerEffect panner;

    const auto panParam = panner.parameter(Constants::NahdXml::xmlKeyPan().toStdString());
    QVERIFY(panParam.has_value());

    // Pan Center: constant-power gives cos(π/4) on both channels
    panParam->get().update(0.5f);
    panner.sync();
    double l = 1.0, r = 1.0;
    panner.process(l, r);
    const double centerAngle = 0.5 * std::numbers::pi * 0.5;
    QCOMPARE(l, std::cos(centerAngle));
    QCOMPARE(r, std::sin(centerAngle));

    // Pan Left: all signal to left
    panParam->get().update(0.0f);
    panner.sync();
    l = 1.0, r = 1.0;
    panner.process(l, r);
    QCOMPARE(l, 1.0);
    QCOMPARE(r, 0.0);

    // Pan Right: all signal to right
    panParam->get().update(1.0f);
    panner.sync();
    l = 1.0, r = 1.0;
    panner.process(l, r);
    QCOMPARE(l, 0.0);
    QCOMPARE(r, 1.0);
}

void PannerTest::test_width_shouldAdjustStereoImage()
{
    PannerEffect panner;
    const auto widthParam = panner.parameter(Constants::NahdXml::xmlKeyWidth().toStdString());
    QVERIFY(widthParam.has_value());

    // Width 100% (Normal): pure side signal passes through, then center pan is applied
    widthParam->get().update(1.0f);
    panner.sync();
    double l = 1.0, r = -1.0;
    panner.process(l, r);
    const double centerAngle = 0.5 * std::numbers::pi * 0.5;
    QCOMPARE(l, std::cos(centerAngle));
    QCOMPARE(r, -std::sin(centerAngle));

    // Width 0% (Mono): pure side signal collapses to silence
    widthParam->get().update(0.0f);
    panner.sync();
    l = 1.0, r = -1.0;
    panner.process(l, r);
    QCOMPARE(l, 0.0);
    QCOMPARE(r, 0.0);

    // Width 0% with pure mid signal: side collapses, mid passes through with center pan
    l = 1.0, r = 1.0;
    panner.process(l, r);
    QCOMPARE(l, std::cos(centerAngle));
    QCOMPARE(r, std::sin(centerAngle));
}

void PannerTest::test_sync_shouldUpdateInternalState()
{
    PannerEffect panner;
    const auto panParam = panner.parameter(Constants::NahdXml::xmlKeyPan().toStdString());
    QVERIFY(panParam.has_value());

    panParam->get().update(0.1f);
    panner.sync();

    // Input (1,0): after MS+width=1 → (1,0); constant-power pan=0.1
    // Use static_cast<double>(0.1f) to match the float→double conversion in sync()
    double l = 1.0, r = 0.0;
    panner.process(l, r);
    const double angle = static_cast<double>(0.1f) * std::numbers::pi * 0.5;
    QCOMPARE(l, std::cos(angle));
    QCOMPARE(r, 0.0);

    // Input (0,1): after MS+width=1 → (0,1); constant-power pan=0.1
    l = 0.0, r = 1.0;
    panner.process(l, r);
    QCOMPARE(l, 0.0);
    QCOMPARE(r, std::sin(angle));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PannerTest)
