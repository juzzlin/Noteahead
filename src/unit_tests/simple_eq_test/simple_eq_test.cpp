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

#include "simple_eq_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/simple_eq.hpp"

#include <QTest>

#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double SampleRate = 44100.0;

void setAmount(SimpleEq & effect, float value)
{
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

SimpleEq makeEq()
{
    SimpleEq effect;
    effect.setSampleRate(SampleRate);
    return effect;
}

//! Steady-state gain in dB applied to a unit sine at the given frequency (see vintage_passive_eq_test).
double measureGainDb(SimpleEq & effect, double frequency)
{
    effect.reset();

    const auto warmupSamples = static_cast<int>(SampleRate * 0.5);
    const auto measureSamples = static_cast<int>(SampleRate * 0.5);
    const double omega = 2.0 * std::numbers::pi * frequency / SampleRate;

    int n = 0;
    for (int i = 0; i < warmupSamples; i++, n++) {
        double l = std::sin(omega * n);
        double r = l;
        effect.process(l, r);
    }

    double sumSquares = 0.0;
    for (int i = 0; i < measureSamples; i++, n++) {
        double l = std::sin(omega * n);
        double r = l;
        effect.process(l, r);
        sumSquares += l * l;
    }

    const double outputRms = std::sqrt(sumSquares / measureSamples);
    const double inputRms = 1.0 / std::numbers::sqrt2;
    return 20.0 * std::log10(outputRms / inputRms);
}

} // namespace

void SimpleEqTest::test_amount_zero_shouldPassThrough()
{
    auto effect = makeEq();
    // Amount at zero: every stage bypasses, so the signal is untouched.
    for (double input = -0.8; input <= 0.8; input += 0.2) {
        double l = input;
        double r = -input;
        effect.process(l, r);
        QVERIFY(qFuzzyCompare(l + 1.0, input + 1.0));
        QVERIFY(qFuzzyCompare(r + 1.0, -input + 1.0));
    }
}

void SimpleEqTest::test_amount_engaged_shouldBoostBodyAndAir()
{
    auto effect = makeEq();
    setAmount(effect, 1.0f);

    // The smile curve lifts both the low body and the high air.
    QVERIFY(measureGainDb(effect, 60.0) > 1.0);
    QVERIFY(measureGainDb(effect, 14000.0) > 1.0);
}

void SimpleEqTest::test_amount_engaged_shouldScoopLowMid()
{
    auto effect = makeEq();
    setAmount(effect, 1.0f);

    // The low-mid mud band is cut, not boosted.
    QVERIFY(measureGainDb(effect, 350.0) < -0.5);
}

void SimpleEqTest::test_amount_higher_shouldApplyStrongerCurve()
{
    auto low = makeEq();
    setAmount(low, 0.3f);

    auto high = makeEq();
    setAmount(high, 1.0f);

    // A hotter Amount pushes the body boost further.
    QVERIFY(measureGainDb(high, 60.0) > measureGainDb(low, 60.0) + 1.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SimpleEqTest)
