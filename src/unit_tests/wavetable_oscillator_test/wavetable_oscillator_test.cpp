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

#include "wavetable_oscillator_test.hpp"

#include "../../domain/dsp/wavetable_oscillator.hpp"

#include <QTest>
#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double SampleRate = 44100.0;
constexpr double Frequency = 261.63;

WavetableOscillator createOscillator(double frequency = Frequency)
{
    // Building the table is by far the most expensive part, and it never changes.
    static const auto wavetable = Wavetable::createClassicSet();

    WavetableOscillator oscillator;
    oscillator.setSampleRate(SampleRate);
    oscillator.setFrequency(frequency);
    oscillator.setWavetable(wavetable);
    return oscillator;
}

} // namespace

void WavetableOscillatorTest::test_position_step_shouldNotStepTheOutput()
{
    // Morph within the sine..triangle half of the table. Neither end has an edge of its own there,
    // so anything that steps the output can only have come from the morph.
    const double from = 0.0;
    const double to = 0.33;

    const auto steadyStep = [](double frequency, double position) {
        auto oscillator = createOscillator(frequency);
        double previous = 0.0;
        double maxStep = 0.0;
        for (int i = 0; i < 2000; i++) {
            oscillator.setPosition(position);
            const auto sample = oscillator.nextSample();
            if (i > 0) {
                maxStep = std::max(maxStep, std::abs(sample - previous));
            }
            previous = sample;
        }
        return maxStep;
    };

    // A low note gets the least glide per cycle and is the hardest case.
    for (const double frequency : { 32.70, 261.63, 2093.00 }) {
        const auto reference = std::max(steadyStep(frequency, from), steadyStep(frequency, to));

        // A sample-and-hold or square LFO moves the position in one jump. Vary how long the
        // oscillator settles first so the jump lands on every phase of the waveform, including the
        // ones where the two ends of the morph differ the most.
        double worstJumpStep = 0.0;
        for (int settleSamples = 2000; settleSamples < 2200; settleSamples++) {
            auto oscillator = createOscillator(frequency);
            double previous = 0.0;
            for (int i = 0; i < settleSamples; i++) {
                oscillator.setPosition(from);
                previous = oscillator.nextSample();
            }
            for (int i = 0; i < 256; i++) {
                oscillator.setPosition(to);
                const auto sample = oscillator.nextSample();
                worstJumpStep = std::max(worstJumpStep, std::abs(sample - previous));
                previous = sample;
            }
        }

        QVERIFY2(worstJumpStep <= reference * 1.5,
                 qPrintable(QString { "At %1 Hz the worst step across the morph jump was %2, steady state is %3" }.arg(frequency).arg(worstJumpStep).arg(reference)));
    }
}

void WavetableOscillatorTest::test_position_step_shouldReachTargetAfterSmoothingTime()
{
    auto oscillator = createOscillator();

    oscillator.setPosition(0.0);
    oscillator.nextSample();

    const double target = 1.0;
    const int samples = static_cast<int>(WavetableOscillator::PositionSmoothingSeconds * SampleRate * 5.0);
    for (int i = 0; i < samples; i++) {
        oscillator.setPosition(target);
        oscillator.nextSample();
    }

    QCOMPARE(oscillator.targetPosition(), target);
    QVERIFY2(std::abs(oscillator.position() - target) < 0.01,
             qPrintable(QString { "Position was %1 after five time constants" }.arg(oscillator.position())));
}

void WavetableOscillatorTest::test_position_firstSet_shouldApplyImmediately()
{
    auto oscillator = createOscillator();

    oscillator.setPosition(0.7);

    QCOMPARE(oscillator.position(), 0.7);
}

void WavetableOscillatorTest::test_position_afterSnap_shouldApplyImmediately()
{
    auto oscillator = createOscillator();

    oscillator.setPosition(0.1);
    oscillator.nextSample();
    oscillator.setPosition(0.8);

    QVERIFY(oscillator.position() < 0.2); // Still gliding

    oscillator.snapPosition();
    oscillator.setPosition(0.8);

    QCOMPARE(oscillator.position(), 0.8);
}

void WavetableOscillatorTest::test_position_outOfRange_shouldClamp()
{
    auto oscillator = createOscillator();

    oscillator.setPosition(-0.5);
    QCOMPARE(oscillator.targetPosition(), 0.0);

    oscillator.snapPosition();
    oscillator.setPosition(1.5);
    QCOMPARE(oscillator.targetPosition(), 1.0);
    QCOMPARE(oscillator.position(), 1.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WavetableOscillatorTest)
