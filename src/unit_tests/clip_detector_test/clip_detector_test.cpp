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

#include "clip_detector_test.hpp"

#include "../../domain/utility/clip_detector.hpp"

#include <QTest>

#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t frameCount = 64;

std::vector<double> buffer(double level)
{
    return std::vector<double>(frameCount * 2, level);
}

} // namespace

void ClipDetectorTest::test_write_belowFullScale_shouldNotLatch()
{
    ClipDetector detector;
    const auto samples = buffer(0.999);
    detector.write(samples.data(), frameCount);

    QVERIFY(!detector.clipped());
}

void ClipDetectorTest::test_write_atFullScale_shouldLatch()
{
    ClipDetector detector;
    auto samples = buffer(0.5);
    // A single sample is enough: a short overshoot is exactly what the indicator exists to report
    samples[frameCount] = 1.0;
    detector.write(samples.data(), frameCount);

    QVERIFY(detector.clipped());
}

void ClipDetectorTest::test_write_negativeFullScale_shouldLatch()
{
    ClipDetector detector;
    auto samples = buffer(-0.5);
    samples[3] = -1.5;
    detector.write(samples.data(), frameCount);

    QVERIFY(detector.clipped());
}

void ClipDetectorTest::test_write_afterLatching_shouldStayLatched()
{
    ClipDetector detector;
    const auto loud = buffer(2.0);
    detector.write(loud.data(), frameCount);

    // Falling back on its own would hide the clip from anyone not watching at that moment
    const auto quiet = buffer(0.1);
    for (int i = 0; i < 100; i++) {
        detector.write(quiet.data(), frameCount);
    }

    QVERIFY(detector.clipped());
}

void ClipDetectorTest::test_clear_shouldUnlatchAndAllowDetectingAgain()
{
    ClipDetector detector;
    const auto loud = buffer(2.0);
    detector.write(loud.data(), frameCount);
    QVERIFY(detector.clipped());

    detector.clear();
    QVERIFY(!detector.clipped());

    detector.write(loud.data(), frameCount);
    QVERIFY(detector.clipped());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ClipDetectorTest)
