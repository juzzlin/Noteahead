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

#include "true_stereo_panner_test.hpp"

#include "../../domain/dsp/true_stereo_panner.hpp"

#include <QTest>
#include <cmath>
#include <numbers>

namespace noteahead {

void TrueStereoPannerTest::test_processMono_center_shouldHaveEqualPower()
{
    TrueStereoPanner panner;
    panner.setPan(0.5);

    double l = 0.0, r = 0.0;
    panner.processMono(1.0, l, r);

    const double angle = 0.5 * std::numbers::pi * 0.5;
    QCOMPARE(l, std::cos(angle));
    QCOMPARE(r, std::sin(angle));
    QVERIFY(std::abs(l * l + r * r - 1.0) < 1e-12);
}

void TrueStereoPannerTest::test_processMono_fullLeft_shouldOutputOnlyLeft()
{
    TrueStereoPanner panner;
    panner.setPan(0.0);

    double l = 0.0, r = 0.0;
    panner.processMono(1.0, l, r);

    QCOMPARE(l, 1.0);
    QCOMPARE(r, 0.0);
}

void TrueStereoPannerTest::test_processMono_fullRight_shouldOutputOnlyRight()
{
    TrueStereoPanner panner;
    panner.setPan(1.0);

    double l = 0.0, r = 0.0;
    panner.processMono(1.0, l, r);

    QCOMPARE(l, 0.0);
    QCOMPARE(r, 1.0);
}

void TrueStereoPannerTest::test_process_center_unityWidth_shouldApplyPanOnly()
{
    TrueStereoPanner panner;
    panner.setPan(0.5);
    panner.setWidth(1.0);

    double l = 1.0, r = 1.0;
    panner.process(l, r);

    const double angle = 0.5 * std::numbers::pi * 0.5;
    QCOMPARE(l, std::cos(angle));
    QCOMPARE(r, std::sin(angle));
}

void TrueStereoPannerTest::test_process_zeroWidth_pureMid_shouldApplyPanOnly()
{
    TrueStereoPanner panner;
    panner.setPan(0.5);
    panner.setWidth(0.0);

    double l = 1.0, r = 1.0;
    panner.process(l, r);

    const double angle = 0.5 * std::numbers::pi * 0.5;
    QCOMPARE(l, std::cos(angle));
    QCOMPARE(r, std::sin(angle));
}

void TrueStereoPannerTest::test_process_zeroWidth_pureSide_shouldCollapseToSilence()
{
    TrueStereoPanner panner;
    panner.setPan(0.5);
    panner.setWidth(0.0);

    double l = 1.0, r = -1.0;
    panner.process(l, r);

    QCOMPARE(l, 0.0);
    QCOMPARE(r, 0.0);
}

void TrueStereoPannerTest::test_process_fullLeft_shouldOutputOnlyLeft()
{
    TrueStereoPanner panner;
    panner.setPan(0.0);
    panner.setWidth(1.0);

    double l = 1.0, r = 1.0;
    panner.process(l, r);

    QCOMPARE(l, 1.0);
    QCOMPARE(r, 0.0);
}

void TrueStereoPannerTest::test_process_fullRight_shouldOutputOnlyRight()
{
    TrueStereoPanner panner;
    panner.setPan(1.0);
    panner.setWidth(1.0);

    double l = 1.0, r = 1.0;
    panner.process(l, r);

    QCOMPARE(l, 0.0);
    QCOMPARE(r, 1.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TrueStereoPannerTest)
