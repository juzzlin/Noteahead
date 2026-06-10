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

#include "lfo_test.hpp"

#include "domain/dsp/lfo.hpp"

#include <QTest>
#include <vector>

namespace noteahead {

void LfoTest::test_randomWaveform_shouldProduceValuesInRange()
{
    Lfo lfo;
    lfo.setSampleRate(44100.0);
    lfo.setFrequency(1.0);
    lfo.setWaveform(Lfo::Waveform::Random);
    lfo.reset();

    for (int i = 0; i < 44100; i++) {
        const double v = lfo.nextSample();
        QVERIFY(v >= -1.0);
        QVERIFY(v <= 1.0);
    }
}

void LfoTest::test_randomWaveform_shouldHoldValueForOneCycle()
{
    Lfo lfo;
    lfo.setSampleRate(100.0);
    lfo.setFrequency(1.0); // 100 samples per cycle
    lfo.setWaveform(Lfo::Waveform::Random);
    lfo.reset();

    // First cycle: all 100 samples must be equal
    const double firstCycleValue = lfo.nextSample();
    for (int i = 1; i < 100; i++) {
        QCOMPARE(lfo.nextSample(), firstCycleValue);
    }

    // Second cycle: all 100 samples must be equal within themselves
    const double secondCycleValue = lfo.nextSample();
    for (int i = 1; i < 100; i++) {
        QCOMPARE(lfo.nextSample(), secondCycleValue);
    }
}

void LfoTest::test_randomWaveform_shouldBeDeterministicAfterReset()
{
    Lfo lfo;
    lfo.setSampleRate(44100.0);
    lfo.setFrequency(440.0);
    lfo.setWaveform(Lfo::Waveform::Random);
    lfo.reset();

    std::vector<double> firstRun;
    firstRun.reserve(1000);
    for (int i = 0; i < 1000; i++) {
        firstRun.push_back(lfo.nextSample());
    }

    lfo.reset();
    for (int i = 0; i < 1000; i++) {
        QCOMPARE(lfo.nextSample(), firstRun[i]);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LfoTest)
