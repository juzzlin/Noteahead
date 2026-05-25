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
#include "../../domain/devices/panner_effect.hpp"

#include <QTest>

namespace noteahead {

void PannerTest::test_pan_shouldAdjustGains()
{
    PannerEffect panner;

    const auto panParam = panner.parameter(Constants::NahdXml::xmlKeyPan().toStdString());
    QVERIFY(panParam.has_value());

    // Pan Center
    panParam->get().update(0.5f);
    panner.sync();
    float l = 1.0f, r = 1.0f;
    panner.process(l, r);
    QCOMPARE(l, 1.0f);
    QCOMPARE(r, 1.0f);

    // Pan Left
    panParam->get().update(0.0f);
    panner.sync();
    l = 1.0f, r = 1.0f;
    panner.process(l, r);
    QCOMPARE(l, 1.0f);
    QCOMPARE(r, 0.0f);

    // Pan Right
    panParam->get().update(1.0f);
    panner.sync();
    l = 1.0f, r = 1.0f;
    panner.process(l, r);
    QCOMPARE(l, 0.0f);
    QCOMPARE(r, 1.0f);
}

void PannerTest::test_width_shouldAdjustStereoImage()
{
    PannerEffect panner;
    const auto widthParam = panner.parameter(Constants::NahdXml::xmlKeyReverbWidth().toStdString());
    QVERIFY(widthParam.has_value());

    // Width 100% (Normal)
    widthParam->get().update(1.0f);
    panner.sync();
    float l = 1.0f, r = -1.0f;
    panner.process(l, r);
    QCOMPARE(l, 1.0f);
    QCOMPARE(r, -1.0f);

    // Width 0% (Mono)
    widthParam->get().update(0.0f);
    panner.sync();
    l = 1.0f, r = -1.0f; // Pure side signal
    panner.process(l, r);
    QCOMPARE(l, 0.0f);
    QCOMPARE(r, 0.0f);

    l = 1.0f, r = 1.0f; // Pure mid signal
    panner.process(l, r);
    QCOMPARE(l, 1.0f);
    QCOMPARE(r, 1.0f);
}

void PannerTest::test_sync_shouldUpdateInternalState()
{
    // Tested implicitly in other tests, but let's be explicit
    PannerEffect panner;
    const auto panParam = panner.parameter(Constants::NahdXml::xmlKeyPan().toStdString());
    QVERIFY(panParam.has_value());

    panParam->get().update(0.1f);
    panner.sync();

    float l = 1.0f, r = 0.0f;
    panner.process(l, r);
    // gainL = min(1.0, 2.0 - 0.1*2.0) = min(1.0, 1.8) = 1.0
    // gainR = min(1.0, 0.1*2.0) = 0.2
    // If input is (1,0), output should be (1,0)
    QCOMPARE(l, 1.0f);
    QCOMPARE(r, 0.0f);

    l = 0.0f, r = 1.0f;
    panner.process(l, r);
    QCOMPARE(l, 0.0f);
    QCOMPARE(r, 0.2f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PannerTest)
