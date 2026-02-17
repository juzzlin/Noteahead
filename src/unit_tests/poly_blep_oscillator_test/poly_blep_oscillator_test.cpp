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

#include "poly_blep_oscillator_test.hpp"
#include "../../domain/dsp/poly_blep_oscillator.hpp"

#include <QTest>
#include <cmath>

namespace noteahead {

void PolyBlepOscillatorTest::test_nextSample_saw_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0); // 100 samples per cycle
    osc.setWaveform(PolyBlepOscillator::Waveform::Saw);

    // Initial phase is 0.0
    // At t=0, saw value is (2*0)-1 = -1.0.
    // PolyBlep correction at t=0: polyBlep(0) = 0 + 0 - 0 - 1 = -1.0
    // value = -1.0 - (-1.0) = 0.0
    QCOMPARE(osc.nextSample(), 0.0);

    // After 25 samples, phase should be 0.25
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    // At t=0.25, saw value is (2*0.25)-1 = -0.5.
    // polyBlep(0.25) is 0 because dt = 441/44100 = 0.01
    QCOMPARE(osc.nextSample(), -0.5);

    // At t=0.5
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.0);

    // At t=0.75
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.5);
}

void PolyBlepOscillatorTest::test_nextSample_square_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);
    osc.setWaveform(PolyBlepOscillator::Waveform::Square);

    // Initial phase 0.0, pw 0.5
    // t=0, value = 1.0 (since 0 < 0.5)
    // polyBlep(0) = -1.0
    // polyBlep(fmod(0+0.5, 1.0)) = polyBlep(0.5) = 0
    // value = 1.0 - (2*0.5 - 1) - 1.0 + 0 = 0.0
    QCOMPARE(osc.nextSample(), 0.0);

    // At t=0.25
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    // value = 1.0 - 0 - 0 + 0 = 1.0
    QCOMPARE(osc.nextSample(), 1.0);

    // At t=0.5
    // value = -1.0 (since 0.5 >= 0.5)
    // polyBlep(0.5) is correction for step at t=0.5
    // Actually polyBlep is only non-zero near 0 and 1.
    // In Square wave:
    // value -= (2.0 * pw - 1.0); // 0
    // value += polyBlep(t);
    // value -= polyBlep(std::fmod(t + (1.0 - pw), 1.0));

    // At t=0.5:
    // value = -1.0
    // polyBlep(0.5) = 0
    // fmod(0.5 + 0.5, 1.0) = 0
    // value = -1.0 + 0 - (-1.0) = 0.0
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.0);
}

void PolyBlepOscillatorTest::test_nextSample_triangle_shouldReturnExpectedValues()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);
    osc.setWaveform(PolyBlepOscillator::Waveform::Triangle);

    // t=0, value = (4*0 - 1) = -1.0
    QCOMPARE(osc.nextSample(), -1.0);

    // t=0.25, value = (4*0.25 - 1) = 0.0
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 0.0);

    // t=0.5, value = (3 - 4*0.5) = 1.0
    for (int i = 0; i < 24; i++) {
        osc.nextSample();
    }
    QCOMPARE(osc.nextSample(), 1.0);
}

void PolyBlepOscillatorTest::test_setFrequency_shouldUpdatePhaseStep()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);

    osc.nextSample();
    QCOMPARE(osc.phase(), 0.01);

    osc.setFrequency(882.0);
    osc.nextSample();
    QCOMPARE(osc.phase(), 0.03);
}

void PolyBlepOscillatorTest::test_sync_shouldResetPhase()
{
    PolyBlepOscillator osc;
    osc.setSampleRate(44100.0);
    osc.setFrequency(441.0);

    osc.nextSample();
    osc.nextSample();
    QVERIFY(osc.phase() > 0.0);

    osc.sync(0.5);
    QCOMPARE(osc.phase(), 0.5);
}

void PolyBlepOscillatorTest::test_setPulseWidth_shouldClampValues()
{
    PolyBlepOscillator osc;

    osc.setPulseWidth(0.5);
    QCOMPARE(osc.pulseWidth(), 0.5);

    osc.setPulseWidth(-1.0);
    QCOMPARE(osc.pulseWidth(), 0.01);

    osc.setPulseWidth(2.0);
    QCOMPARE(osc.pulseWidth(), 0.99);
}

void PolyBlepOscillatorTest::test_setShape_shouldClampValues()
{
    PolyBlepOscillator osc;

    osc.setShape(0.5);
    QCOMPARE(osc.shape(), 0.5);

    osc.setShape(-1.0);
    QCOMPARE(osc.shape(), 0.0);

    osc.setShape(2.0);
    QCOMPARE(osc.shape(), 1.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PolyBlepOscillatorTest)
