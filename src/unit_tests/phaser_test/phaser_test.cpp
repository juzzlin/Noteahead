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

#include "phaser_test.hpp"

#include "../../domain/dsp/phaser.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 44100.0 };

double toneAt(int frame, double frequency)
{
    return std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(frame) / SampleRate);
}

double rms(const std::vector<double> & samples)
{
    double sum = 0.0;
    for (const auto sample : samples) {
        sum += sample * sample;
    }
    return std::sqrt(sum / static_cast<double>(samples.size()));
}

} // namespace

void PhaserTest::test_process_disabled_shouldPassThrough()
{
    Phaser phaser;
    phaser.setSampleRate(SampleRate);
    phaser.setEnabled(false);

    for (int frame = 0; frame < 1000; frame++) {
        const double input = toneAt(frame, 440.0);
        double left = input;
        double right = input;
        phaser.process(left, right);
        QCOMPARE(left, input);
        QCOMPARE(right, input);
    }
}

void PhaserTest::test_process_enabled_shouldAlterSignal()
{
    Phaser phaser;
    phaser.setSampleRate(SampleRate);
    phaser.setEnabled(true);
    phaser.setColor(0.5);
    phaser.setRate(0.3);

    bool altered = false;
    for (int frame = 0; frame < 2000; frame++) {
        const double input = toneAt(frame, 440.0);
        double left = input;
        double right = input;
        phaser.process(left, right);
        if (std::abs(left - input) > 0.01) {
            altered = true;
        }
    }

    QVERIFY(altered);
}

void PhaserTest::test_process_enabled_shouldSweepOverTime()
{
    // A static all-pass cascade would attenuate a given tone by a fixed amount. The sweep moves the
    // notches, so the same tone comes out at different levels at different points of the cycle.
    Phaser phaser;
    phaser.setSampleRate(SampleRate);
    phaser.setEnabled(true);
    phaser.setColor(0.8);
    phaser.setRate(1.0);

    std::vector<double> firstWindow;
    std::vector<double> secondWindow;
    const int windowLength = 2000;
    for (int frame = 0; frame < windowLength * 4; frame++) {
        double left = toneAt(frame, 700.0);
        double right = left;
        phaser.process(left, right);
        if (frame < windowLength) {
            firstWindow.push_back(left);
        } else if (frame >= windowLength * 3) {
            secondWindow.push_back(left);
        }
    }

    QVERIFY(std::abs(rms(firstWindow) - rms(secondWindow)) > 0.01);
}

void PhaserTest::test_process_enabled_shouldSeparateChannels()
{
    // The channels run in quadrature, so a mono input comes out as a moving stereo image.
    Phaser phaser;
    phaser.setSampleRate(SampleRate);
    phaser.setEnabled(true);
    phaser.setColor(0.6);
    phaser.setRate(0.5);

    double difference = 0.0;
    for (int frame = 0; frame < 4000; frame++) {
        double left = toneAt(frame, 500.0);
        double right = left;
        phaser.process(left, right);
        difference = std::max(difference, std::abs(left - right));
    }

    QVERIFY(difference > 0.01);
}

void PhaserTest::test_process_maximumColor_shouldStayStable()
{
    Phaser phaser;
    phaser.setSampleRate(SampleRate);
    phaser.setEnabled(true);
    phaser.setColor(1.0);
    phaser.setRate(1.0);

    for (int frame = 0; frame < 200000; frame++) {
        double left = toneAt(frame, 220.0);
        double right = left;
        phaser.process(left, right);
        QVERIFY(std::isfinite(left));
        QVERIFY(std::isfinite(right));
        QVERIFY(std::abs(left) < 10.0);
        QVERIFY(std::abs(right) < 10.0);
    }
}

void PhaserTest::test_reset_shouldClearState()
{
    Phaser phaser;
    phaser.setSampleRate(SampleRate);
    phaser.setEnabled(true);
    phaser.setColor(0.5);
    phaser.setRate(0.3);

    std::vector<double> before;
    for (int frame = 0; frame < 512; frame++) {
        double left = toneAt(frame, 440.0);
        double right = left;
        phaser.process(left, right);
        before.push_back(left);
    }

    phaser.reset();

    std::vector<double> after;
    for (int frame = 0; frame < 512; frame++) {
        double left = toneAt(frame, 440.0);
        double right = left;
        phaser.process(left, right);
        after.push_back(left);
    }

    QCOMPARE(before, after);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PhaserTest)
