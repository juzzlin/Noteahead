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

#include "sweep_envelope_test.hpp"

#include "../../domain/dsp/sweep_envelope.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

//! Number of samples until the sweep stops running, capped at @p limit.
int samplesUntilStopped(SweepEnvelope & sweep, int limit)
{
    int count = 0;
    while (sweep.isRunning() && count < limit) {
        sweep.nextSample();
        count++;
    }
    return count;
}

} // namespace

void SweepEnvelopeTest::test_sweepEnvelope_lengthBeats_shouldEndOnTheBeat_data()
{
    QTest::addColumn<double>("bpm");
    QTest::addColumn<double>("beats");

    QTest::newRow("120 bpm, 2 bars") << 120.0 << 8.0;
    QTest::newRow("137 bpm, 8 bars") << 137.0 << 32.0;
    QTest::newRow("174 bpm, 32 bars") << 174.0 << 128.0;
}

void SweepEnvelopeTest::test_sweepEnvelope_lengthBeats_shouldEndOnTheBeat()
{
    QFETCH(double, bpm);
    QFETCH(double, beats);

    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setBpm(bpm);
    sweep.setLengthBeats(beats);
    sweep.trigger();

    const int expected = static_cast<int>(std::round(beats * 60.0 / bpm * SampleRate));
    const int actual = samplesUntilStopped(sweep, expected * 2);
    QVERIFY2(std::abs(actual - expected) <= 1, qPrintable(QString { "%1 vs %2" }.arg(actual).arg(expected)));
    QCOMPARE(sweep.nextSample(), 1.0);
}

void SweepEnvelopeTest::test_sweepEnvelope_lengthBeats_tempoChange_shouldApplyToRemainder()
{
    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setBpm(120.0);
    sweep.setLengthBeats(8.0);
    sweep.trigger();

    // Four beats at 120 bpm, then the other four at 60 bpm.
    const int firstHalf = static_cast<int>(SampleRate * 2.0);
    for (int n = 0; n < firstHalf; n++) {
        sweep.nextSample();
    }
    QVERIFY(std::abs(sweep.progress() - 0.5) < 1e-9);

    sweep.setBpm(60.0);
    const int secondHalf = samplesUntilStopped(sweep, firstHalf * 4);
    QVERIFY2(std::abs(secondHalf - static_cast<int>(SampleRate * 4.0)) <= 1, qPrintable(QString::number(secondHalf)));
}

void SweepEnvelopeTest::test_sweepEnvelope_lengthSeconds_shouldEndOnTime()
{
    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setLengthSeconds(1.5);
    sweep.setBpm(90.0);
    sweep.trigger();

    QVERIFY(std::abs(samplesUntilStopped(sweep, 200000) - 72000) <= 1);
}

void SweepEnvelopeTest::test_sweepEnvelope_curve_shouldShapeMidpoint_data()
{
    QTest::addColumn<double>("curve");
    QTest::addColumn<double>("expected");

    QTest::newRow("linear") << 0.0 << 0.5;
    QTest::newRow("exponential") << 1.0 << 1.0 / (std::exp(3.0) + 1.0);
    QTest::newRow("logarithmic") << -1.0 << 1.0 - 1.0 / (std::exp(3.0) + 1.0);
}

void SweepEnvelopeTest::test_sweepEnvelope_curve_shouldShapeMidpoint()
{
    QFETCH(double, curve);
    QFETCH(double, expected);

    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setStartValue(0.0);
    sweep.setEndValue(1.0);
    sweep.setCurve(curve);
    sweep.setLengthSeconds(1.0);
    sweep.trigger();

    for (int n = 0; n < 24000; n++) {
        sweep.nextSample();
    }
    QVERIFY(std::abs(sweep.nextSample() - expected) < 1e-6);
}

void SweepEnvelopeTest::test_sweepEnvelope_modeHold_shouldStayAtEnd()
{
    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setStartValue(200.0);
    sweep.setEndValue(8000.0);
    sweep.setLengthSeconds(0.1);
    sweep.trigger();

    samplesUntilStopped(sweep, 10000);
    for (int n = 0; n < 1000; n++) {
        QCOMPARE(sweep.nextSample(), 8000.0);
    }
    QVERIFY(!sweep.isRunning());
}

void SweepEnvelopeTest::test_sweepEnvelope_modeLoop_shouldRestartFromStart()
{
    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setMode(SweepEnvelope::Mode::Loop);
    sweep.setStartValue(-1.0);
    sweep.setEndValue(1.0);
    sweep.setLengthSeconds(0.01);
    sweep.trigger();

    const int length = 480;
    for (int cycle = 0; cycle < 5; cycle++) {
        QVERIFY(std::abs(sweep.nextSample() + 1.0) < 1e-6);
        for (int n = 1; n < length; n++) {
            sweep.nextSample();
        }
    }
    QVERIFY(sweep.isRunning());
}

void SweepEnvelopeTest::test_sweepEnvelope_modePingPong_shouldReturnToStart()
{
    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setMode(SweepEnvelope::Mode::PingPong);
    sweep.setLengthSeconds(0.01);
    sweep.trigger();

    const int length = 480;
    for (int n = 0; n < length; n++) {
        sweep.nextSample();
    }
    QVERIFY(std::abs(sweep.nextSample() - 1.0) < 1e-6);
    for (int n = 1; n < length; n++) {
        sweep.nextSample();
    }
    QVERIFY(std::abs(sweep.nextSample()) < 1e-6);
    QVERIFY(sweep.isRunning());
}

void SweepEnvelopeTest::test_sweepEnvelope_untriggered_shouldStayAtStart()
{
    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setStartValue(0.25);

    for (int n = 0; n < 1000; n++) {
        QCOMPARE(sweep.nextSample(), 0.25);
    }
    QVERIFY(!sweep.isRunning());
}

void SweepEnvelopeTest::test_sweepEnvelope_lengthZero_shouldJumpToEnd()
{
    SweepEnvelope sweep;
    sweep.setSampleRate(SampleRate);
    sweep.setLengthSeconds(0.0);
    sweep.trigger();

    QCOMPARE(sweep.nextSample(), 0.0);
    QCOMPARE(sweep.nextSample(), 1.0);
    QVERIFY(!sweep.isRunning());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SweepEnvelopeTest)
