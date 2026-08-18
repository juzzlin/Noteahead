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

#include "stereo_field_renderer_test.hpp"

#include "../../view/qml/Dialogs/stereo_field_renderer.hpp"

#include <QTest>

namespace noteahead {

namespace {

// The classes, widest first, by a correlation comfortably inside each of them.
constexpr int phaseRisk = 0;
constexpr int veryWide = 1;
constexpr int wide = 2;
constexpr int medium = 3;
constexpr int narrow = 4;
constexpr int mono = 5;

//! Settles the classifier on a reading, the way a steady passage would.
int settle(double correlation, int from = mono)
{
    int current = from;
    for (int i = 0; i < 8; i++) {
        current = StereoFieldRenderer::nextWidthClass(current, correlation);
    }
    return current;
}

} // namespace

void StereoFieldRendererTest::test_widthClass_settledInput_shouldNameEveryClass()
{
    QCOMPARE(settle(-0.50), phaseRisk);
    QCOMPARE(settle(0.10), veryWide);
    QCOMPARE(settle(0.35), wide);
    QCOMPARE(settle(0.65), medium);
    QCOMPARE(settle(0.88), narrow);
    QCOMPARE(settle(0.99), mono);
}

void StereoFieldRendererTest::test_widthClass_atABoundary_shouldNotChangeUntilTheMarginIsPassed()
{
    // Settled just above the medium/narrow boundary at 0.8.
    const int current = settle(0.85);
    QCOMPARE(current, narrow);

    // Dipping below the boundary is not enough on its own.
    QCOMPARE(StereoFieldRenderer::nextWidthClass(current, 0.79), narrow);
    QCOMPARE(StereoFieldRenderer::nextWidthClass(current, 0.78), narrow);

    // Travelling a margin past it is.
    QCOMPARE(StereoFieldRenderer::nextWidthClass(current, 0.76), medium);
}

void StereoFieldRendererTest::test_widthClass_wanderingOnABoundary_shouldNotChangeAtAll()
{
    // What a sustained pad sitting exactly on a boundary looks like: the reading jitters either
    // side of it and the word must not follow, because a word that changes thirty times a second
    // is one that cannot be read.
    int current = settle(0.52);
    QCOMPARE(current, medium);

    const std::vector<double> wander { 0.505, 0.495, 0.502, 0.498, 0.51, 0.49, 0.5, 0.497 };
    for (int pass = 0; pass < 4; pass++) {
        for (const double value : wander) {
            current = StereoFieldRenderer::nextWidthClass(current, value);
            QCOMPARE(current, medium);
        }
    }
}

void StereoFieldRendererTest::test_widthClass_aLongWayPastABoundary_shouldSkipToTheRightClass()
{
    // Hysteresis holds a class against jitter, not against the signal actually changing.
    QCOMPARE(StereoFieldRenderer::nextWidthClass(mono, -0.4), phaseRisk);
    QCOMPARE(StereoFieldRenderer::nextWidthClass(phaseRisk, 0.99), mono);
}

void StereoFieldRendererTest::test_phaseRisk_crossingZero_shouldLatchUntilTheMarginIsPassed()
{
    QVERIFY(!StereoFieldRenderer::nextPhaseRisk(false, 0.01));
    QVERIFY(StereoFieldRenderer::nextPhaseRisk(false, -0.01));

    // Once flagged it stays flagged until the reading is clear of zero, not merely back at it.
    QVERIFY(StereoFieldRenderer::nextPhaseRisk(true, 0.01));
    QVERIFY(!StereoFieldRenderer::nextPhaseRisk(true, 0.05));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StereoFieldRendererTest)
