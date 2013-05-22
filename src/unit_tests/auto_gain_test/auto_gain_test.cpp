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

#include "auto_gain_test.hpp"

#include "../../domain/dsp/auto_gain.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

AutoGain::Grid uniformGrid(double db)
{
    AutoGain::Grid grid {};
    grid.fill(db);
    return grid;
}

double toDb(double gain)
{
    return 20.0 * std::log10(gain);
}

} // namespace

void AutoGainTest::test_compensationDb_flat_shouldBeZero()
{
    QCOMPARE(AutoGain::compensationDb(uniformGrid(0.0)), 0.0);
}

void AutoGainTest::test_compensationDb_uniformBoost_shouldCancelIt()
{
    QVERIFY(std::abs(AutoGain::compensationDb(uniformGrid(6.0)) + 6.0) < 1e-9);
    QVERIFY(std::abs(AutoGain::compensationDb(uniformGrid(-4.0)) - 4.0) < 1e-9);
}

void AutoGainTest::test_compensationDb_narrowBoost_shouldCutLittle()
{
    // +12 dB on two points of 64, about a third of an octave: loud where it is, but most of the
    // spectrum is untouched, and so is most of the level
    auto grid = uniformGrid(0.0);
    grid.at(30) = 12.0;
    grid.at(31) = 12.0;
    const double db = AutoGain::compensationDb(grid);
    QVERIFY2(db < -0.5 && db > -2.0, qPrintable(QString::number(db)));
}

void AutoGainTest::test_compensationDb_extreme_shouldBeClamped()
{
    QCOMPARE(AutoGain::compensationDb(uniformGrid(60.0)), -24.0);
    QCOMPARE(AutoGain::compensationDb(uniformGrid(-60.0)), 24.0);
}

void AutoGainTest::test_gridFrequencies_lowSampleRate_shouldStayBelowNyquist()
{
    const auto standard = AutoGain::gridFrequencies(48000.0);
    QCOMPARE(standard.front(), 20.0);
    QVERIFY(std::abs(standard.back() - 20000.0) < 1e-6);

    const auto low = AutoGain::gridFrequencies(22050.0);
    QVERIFY(low.back() < 11025.0);
}

void AutoGainTest::test_next_atUnity_shouldBeExactlyOne()
{
    AutoGain gain;
    gain.setTargetDb(0.0, 48000.0);
    for (int i = 0; i < 2000; i++) {
        QCOMPARE(gain.next(), 1.0);
    }
}

void AutoGainTest::test_next_newTarget_shouldRampAndLandOnIt()
{
    AutoGain gain;
    gain.setTargetDb(-6.0, 48000.0);
    const double target = std::pow(10.0, -6.0 / 20.0);

    // Part way after 10 ms, there after 20 ms, and exactly the target from then on
    double previous = 1.0;
    for (int i = 0; i < 480; i++) {
        const double current = gain.next();
        QVERIFY(current < previous);
        previous = current;
    }
    QVERIFY(toDb(previous) < -2.0 && toDb(previous) > -4.0);
    for (int i = 480; i < 960; i++) {
        previous = gain.next();
    }
    QCOMPARE(previous, target);
    QCOMPARE(gain.next(), target);
}

void AutoGainTest::test_setTargetDb_noSampleRate_shouldJump()
{
    AutoGain gain;
    gain.setTargetDb(-6.0, 0.0);
    QCOMPARE(gain.next(), std::pow(10.0, -6.0 / 20.0));
}

void AutoGainTest::test_reset_midRamp_shouldJumpToTarget()
{
    AutoGain gain;
    gain.setTargetDb(6.0, 48000.0);
    gain.next();
    gain.reset();
    QCOMPARE(gain.next(), std::pow(10.0, 6.0 / 20.0));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutoGainTest)
