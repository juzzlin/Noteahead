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

#include "juno_chorus_test.hpp"

#include "../../domain/dsp/juno_chorus.hpp"

#include <QTest>

#include <cmath>
#include <random>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

const std::array<JunoChorus::Mode, 3> onModes { JunoChorus::Mode::I, JunoChorus::Mode::II, JunoChorus::Mode::Both };

} // namespace

void JunoChorusTest::test_process_off_shouldPassTheInputToBothSides()
{
    JunoChorus chorus;
    chorus.setSampleRate(SampleRate);
    for (int i = 0; i < 1000; i++) {
        const double input = std::sin(i * 0.05);
        double left = 0.0;
        double right = 0.0;
        chorus.process(input, left, right);
        QCOMPARE(left, input);
        QCOMPARE(right, input);
    }
}

void JunoChorusTest::test_process_stereoOn_shouldKeepEachSideOnItsOwnLine()
{
    // A voice panned hard left stays there: the right is its own line and hears nothing of it
    for (auto && mode : onModes) {
        JunoChorus chorus;
        chorus.setSampleRate(SampleRate);
        chorus.setMode(mode);
        double leftEnergy = 0.0;
        for (int i = 0; i < 48000; i++) {
            double left = 0.0;
            double right = 0.0;
            chorus.process(std::sin(i * 0.05), 0.0, left, right);
            QCOMPARE(right, 0.0);
            leftEnergy += left * left;
        }
        QVERIFY(leftEnergy > 1000.0);
    }
}

void JunoChorusTest::test_delayRange_modes_shouldMatchTheJuno60()
{
    QCOMPARE(JunoChorus::minDelayMs(JunoChorus::Mode::I), 1.66);
    QCOMPARE(JunoChorus::maxDelayMs(JunoChorus::Mode::I), 5.35);
    QCOMPARE(JunoChorus::minDelayMs(JunoChorus::Mode::II), 1.66);
    QCOMPARE(JunoChorus::maxDelayMs(JunoChorus::Mode::II), 5.35);
    QCOMPARE(JunoChorus::minDelayMs(JunoChorus::Mode::Both), 3.3);
    QCOMPARE(JunoChorus::maxDelayMs(JunoChorus::Mode::Both), 3.7);
    QCOMPARE(JunoChorus::rateHz(JunoChorus::Mode::I), 0.513);
    QCOMPARE(JunoChorus::rateHz(JunoChorus::Mode::II), 0.863);
    QCOMPARE(JunoChorus::rateHz(JunoChorus::Mode::Both), 9.75);
}

void JunoChorusTest::test_sweep_modes_shouldStayInRangeAndMirror()
{
    for (auto && mode : onModes) {
        JunoChorus chorus;
        chorus.setSampleRate(SampleRate);
        chorus.setMode(mode);
        const double centre = 0.5 * (JunoChorus::minDelayMs(mode) + JunoChorus::maxDelayMs(mode));
        double lowest = 1.0e9;
        double highest = 0.0;
        // Three seconds is more than one cycle of the slowest mode
        for (int i = 0; i < static_cast<int>(3.0 * SampleRate); i++) {
            double left = 0.0;
            double right = 0.0;
            chorus.process(0.0, left, right);
            const auto [leftMs, rightMs] = chorus.currentDelaysMs();
            // The right-hand delay runs on the inverted LFO, so the two always straddle the centre
            QVERIFY(std::abs(0.5 * (leftMs + rightMs) - centre) < 1.0e-9);
            lowest = std::min(lowest, leftMs);
            highest = std::max(highest, leftMs);
        }
        QVERIFY2(std::abs(lowest - JunoChorus::minDelayMs(mode)) < 0.01, qPrintable(QString::number(lowest)));
        QVERIFY2(std::abs(highest - JunoChorus::maxDelayMs(mode)) < 0.01, qPrintable(QString::number(highest)));
    }
}

void JunoChorusTest::test_sweep_modes_shouldRunAtTheModeRate()
{
    for (auto && mode : onModes) {
        JunoChorus chorus;
        chorus.setSampleRate(SampleRate);
        chorus.setMode(mode);
        const double centre = 0.5 * (JunoChorus::minDelayMs(mode) + JunoChorus::maxDelayMs(mode));
        constexpr double seconds { 20.0 };
        int upwardCrossings = 0;
        double previous = 0.0;
        for (int i = 0; i < static_cast<int>(seconds * SampleRate); i++) {
            double left = 0.0;
            double right = 0.0;
            chorus.process(0.0, left, right);
            const double offset = chorus.currentDelaysMs().first - centre;
            if (i > 0 && previous < 0.0 && offset >= 0.0) {
                upwardCrossings++;
            }
            previous = offset;
        }
        const double expected = JunoChorus::rateHz(mode) * seconds;
        QVERIFY2(std::abs(upwardCrossings - expected) <= 1.0, qPrintable(QString { "%1 crossings, expected %2" }.arg(upwardCrossings).arg(expected)));
    }
}

void JunoChorusTest::test_process_on_shouldSpreadTheSides()
{
    for (auto && mode : onModes) {
        JunoChorus chorus;
        chorus.setSampleRate(SampleRate);
        chorus.setMode(mode);
        double difference = 0.0;
        double level = 0.0;
        for (int i = 0; i < static_cast<int>(SampleRate); i++) {
            double left = 0.0;
            double right = 0.0;
            chorus.process(std::sin(2.0 * M_PI * 1000.0 * i / SampleRate), left, right);
            difference += std::abs(left - right);
            level += std::abs(left);
        }
        QVERIFY2(difference / level > 0.05, qPrintable(QString::number(difference / level)));
    }
}

void JunoChorusTest::test_process_on_shouldKeepTheLevel()
{
    // Noise, so the comparison is not at the mercy of one frequency landing on a comb notch, and
    // rolled off above 2 kHz the way a synth patch is: white noise puts half its energy above the
    // bucket brigades' filters, which take it out of the delayed copy and nothing else.
    const auto rmsDb = [](JunoChorus::Mode mode) {
        JunoChorus chorus;
        chorus.setSampleRate(SampleRate);
        chorus.setMode(mode);
        std::mt19937 rng { 7 };
        std::uniform_real_distribution<double> noise { -0.5, 0.5 };
        double lowPassed = 0.0;
        const double coefficient = 1.0 - std::exp(-2.0 * M_PI * 2000.0 / SampleRate);
        double sum = 0.0;
        const int frames = static_cast<int>(2.0 * SampleRate);
        for (int i = 0; i < frames; i++) {
            double left = 0.0;
            double right = 0.0;
            lowPassed += coefficient * (noise(rng) - lowPassed);
            chorus.process(lowPassed, left, right);
            sum += left * left + right * right;
        }
        return 10.0 * std::log10(sum / (2.0 * frames));
    };

    const double off = rmsDb(JunoChorus::Mode::Off);
    for (auto && mode : onModes) {
        const double on = rmsDb(mode);
        QVERIFY2(std::abs(on - off) < 1.5, qPrintable(QString { "off %1 dB, on %2 dB" }.arg(off).arg(on)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::JunoChorusTest)
