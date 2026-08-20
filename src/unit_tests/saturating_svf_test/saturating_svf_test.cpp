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

#include "saturating_svf_test.hpp"

#include "../../domain/dsp/saturating_svf.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

//! Cycles rendered per measurement. The first half is discarded so the filter has settled -- a
//! resonant one rings for a good while -- and the second half is what gets measured.
constexpr int Periods = 400;

//! Amplitude of the fundamental at the output, per unit of input.
//!
//! Resolved on the input's own bin, so the harmonics the saturator generates are excluded and what
//! comes back is the gain the filter is giving that tone, which is the quantity these tests are
//! about.
double gainAt(SaturatingSvf & filter, double frequency, double amplitude)
{
    filter.setSampleRate(SampleRate);
    filter.reset();

    const auto total = static_cast<size_t>(std::round(static_cast<double>(Periods) * SampleRate / frequency));
    const size_t settle = total / 2;

    double re = 0.0;
    double im = 0.0;
    const auto measured = static_cast<double>(total - settle);

    for (size_t i = 0; i < total; i++) {
        const double phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate;
        const double out = filter.process(amplitude * std::sin(phase));
        if (i >= settle) {
            re += out * std::cos(phase);
            im += out * std::sin(phase);
        }
    }

    return 2.0 * std::hypot(re, im) / measured / amplitude;
}

} // namespace

void SaturatingSvfTest::test_wellBelowCorner_shouldPassThrough()
{
    SaturatingSvf filter;
    filter.setCutoff(2000.0);
    filter.setResonance(0.0);
    filter.setSaturation(0.0);

    QVERIFY(std::abs(gainAt(filter, 100.0, 0.1) - 1.0) < 0.05);
}

void SaturatingSvfTest::test_wellAboveCorner_shouldAttenuate()
{
    SaturatingSvf filter;
    filter.setCutoff(500.0);
    filter.setResonance(0.0);
    filter.setSaturation(0.0);

    // Two poles: two octaves up is about 24 dB down, so anything near unity would mean the filter
    // is not filtering.
    QVERIFY(gainAt(filter, 2000.0, 0.1) < 0.1);
}

void SaturatingSvfTest::test_resonance_shouldLiftTheCorner()
{
    SaturatingSvf flat;
    flat.setCutoff(1000.0);
    flat.setResonance(0.0);
    flat.setSaturation(0.0);
    const double flatGain = gainAt(flat, 1000.0, 0.05);

    SaturatingSvf resonant;
    resonant.setCutoff(1000.0);
    resonant.setResonance(0.9);
    resonant.setSaturation(0.0);
    const double resonantGain = gainAt(resonant, 1000.0, 0.05);

    QVERIFY(resonantGain > flatGain * 3.0);
}

void SaturatingSvfTest::test_linear_levelChange_shouldNotChangeGain()
{
    // The control for the test below: with the saturator out of the way the filter is linear, so
    // the gain at the corner is the same whatever the input level is.
    SaturatingSvf filter;
    filter.setCutoff(1000.0);
    filter.setResonance(0.9);
    filter.setSaturation(0.0);

    const double quiet = gainAt(filter, 1000.0, 0.01);
    const double loud = gainAt(filter, 1000.0, 1.0);

    QVERIFY(std::abs(loud / quiet - 1.0) < 0.01);
}

void SaturatingSvfTest::test_saturated_resonantPeak_shouldCompressWithLevel()
{
    // The whole point of the class. The integrators cannot exceed the saturator's ceiling, so the
    // resonant peak folds down as the filter is driven harder: the gain a loud tone gets at the
    // corner is well below the gain a quiet one gets. A distortion in front of a linear filter
    // cannot do this -- there the peak keeps its height however hard the input is pushed.
    SaturatingSvf filter;
    filter.setCutoff(1000.0);
    filter.setResonance(0.9);
    filter.setSaturation(4.0);

    const double quiet = gainAt(filter, 1000.0, 0.005);
    const double loud = gainAt(filter, 1000.0, 1.0);

    QVERIFY(loud < quiet * 0.5);
}

void SaturatingSvfTest::test_saturated_output_shouldStayBounded()
{
    // Resonance at the top, driven far past anything the rack would ever hand it. The saturator is
    // what keeps the loop from running away, so this must stay finite and sane.
    SaturatingSvf filter;
    filter.setSampleRate(SampleRate);
    filter.setCutoff(1000.0);
    filter.setResonance(1.0);
    filter.setSaturation(8.0);

    double peak = 0.0;
    for (size_t i = 0; i < 48000; i++) {
        const double phase = 2.0 * std::numbers::pi * 1000.0 * static_cast<double>(i) / SampleRate;
        const double out = filter.process(20.0 * std::sin(phase));
        QVERIFY(std::isfinite(out));
        peak = std::max(peak, std::abs(out));
    }

    QVERIFY(peak < 10.0);
}

void SaturatingSvfTest::test_reset_shouldClearState()
{
    SaturatingSvf filter;
    filter.setSampleRate(SampleRate);
    filter.setCutoff(1000.0);
    filter.setResonance(0.9);
    filter.setSaturation(1.0);

    for (size_t i = 0; i < 1000; i++) {
        filter.process(0.5);
    }
    QVERIFY(std::abs(filter.lowPass()) > 1.0e-6);

    filter.reset();

    QCOMPARE(filter.lowPass(), 0.0);
    QCOMPARE(filter.bandPass(), 0.0);
    QCOMPARE(filter.highPass(), 0.0);
    // A reset filter must start from silence rather than from wherever the last note left it.
    QCOMPARE(filter.process(0.0), 0.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SaturatingSvfTest)
