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

#include "pan_aware_width_test.hpp"

#include "../../domain/dsp/pan_aware_width.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t FrameCount { 64 };
constexpr double Tolerance { 1.0e-12 };

//! Uncorrelated left and right, so there is both mid and side to work on.
std::vector<double> makeStereo()
{
    std::vector<double> buffer(static_cast<size_t>(FrameCount) * 2);
    for (uint32_t i = 0; i < FrameCount; i++) {
        buffer[i * 2] = std::sin(0.3 * static_cast<double>(i));
        buffer[i * 2 + 1] = 0.7 * std::cos(0.11 * static_cast<double>(i) + 0.5);
    }
    return buffer;
}

double leftGain(double pan)
{
    return std::cos(pan * std::numbers::pi * 0.5);
}

double rightGain(double pan)
{
    return std::sin(pan * std::numbers::pi * 0.5);
}

} // namespace

void PanAwareWidthTest::test_process_fullWidth_shouldLeaveBufferUntouched()
{
    PanAwareWidth width;
    auto buffer = makeStereo();
    const auto dry = buffer;
    width.process(buffer, FrameCount, 1.0, 0.3);
    QCOMPARE(buffer, dry);
}

void PanAwareWidthTest::test_process_zeroWidth_shouldBeMonoAtThePanPosition()
{
    constexpr double pan = 0.7;
    PanAwareWidth width;
    auto buffer = makeStereo();
    width.process(buffer, FrameCount, 0.0, pan);
    for (uint32_t i = 0; i < FrameCount; i++) {
        QVERIFY(std::abs(buffer[i * 2] * rightGain(pan) - buffer[i * 2 + 1] * leftGain(pan)) < Tolerance);
    }
}

void PanAwareWidthTest::test_process_zeroWidth_hardLeft_shouldStayHardLeft()
{
    // Mid/side width would pull this back to the centre
    PanAwareWidth width;
    std::vector<double> buffer(static_cast<size_t>(FrameCount) * 2, 0.0);
    for (uint32_t i = 0; i < FrameCount; i++) {
        buffer[i * 2] = std::sin(0.3 * static_cast<double>(i));
    }
    const auto dry = buffer;
    width.process(buffer, FrameCount, 0.0, 0.0);
    for (size_t i = 0; i < buffer.size(); i++) {
        QVERIFY(std::abs(buffer[i] - dry[i]) < Tolerance);
    }
}

void PanAwareWidthTest::test_process_centre_shouldMatchMidSideWidth()
{
    constexpr double w = 0.4;
    PanAwareWidth width;
    auto buffer = makeStereo();
    const auto dry = buffer;
    width.process(buffer, FrameCount, w, 0.5);
    for (uint32_t i = 0; i < FrameCount; i++) {
        const double mid = (dry[i * 2] + dry[i * 2 + 1]) * 0.5;
        const double side = (dry[i * 2] - dry[i * 2 + 1]) * 0.5;
        QVERIFY(std::abs(buffer[i * 2] - (mid + side * w)) < Tolerance);
        QVERIFY(std::abs(buffer[i * 2 + 1] - (mid - side * w)) < Tolerance);
    }
}

void PanAwareWidthTest::test_process_monoAtThePanPosition_shouldNotChangeWithWidth()
{
    // What a mono instrument panned by the device looks like: nothing off the pan direction to remove
    constexpr double pan = 0.25;
    PanAwareWidth width;
    std::vector<double> buffer(static_cast<size_t>(FrameCount) * 2);
    for (uint32_t i = 0; i < FrameCount; i++) {
        const double mono = std::sin(0.2 * static_cast<double>(i));
        buffer[i * 2] = mono * leftGain(pan);
        buffer[i * 2 + 1] = mono * rightGain(pan);
    }
    const auto dry = buffer;
    width.process(buffer, FrameCount, 0.3, pan);
    for (size_t i = 0; i < buffer.size(); i++) {
        QVERIFY(std::abs(buffer[i] - dry[i]) < Tolerance);
    }
}

void PanAwareWidthTest::test_process_widthChange_shouldRampOverTheBlock()
{
    PanAwareWidth width;
    auto first = makeStereo();
    width.process(first, FrameCount, 1.0, 0.5);

    // Pure side, so the output is the width itself at every frame
    std::vector<double> buffer(static_cast<size_t>(FrameCount) * 2);
    for (uint32_t i = 0; i < FrameCount; i++) {
        buffer[i * 2] = 1.0;
        buffer[i * 2 + 1] = -1.0;
    }
    width.process(buffer, FrameCount, 0.0, 0.5);

    QVERIFY(buffer[0] > 0.9);
    QVERIFY(std::abs(buffer[(FrameCount - 1) * 2]) < Tolerance);
    for (uint32_t i = 1; i < FrameCount; i++) {
        QVERIFY(buffer[i * 2] < buffer[(i - 1) * 2]);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PanAwareWidthTest)
