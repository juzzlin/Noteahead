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
#include "../../domain/dsp/svf_filter.hpp"
#include "../../domain/effects/simple_eq.hpp"

#include <QTest>

#include <array>
#include <cmath>
#include <numbers>
#include <random>

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

void setParameter(SimpleEq & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

//! Output over input level, in dB, of four seconds of pink noise through @p effect. The first
//! second is left out, for the filters and the gain ramp to settle. The noise is cut below 20 Hz,
//! where auto gain does not look: the generator puts another octave there, and a low shelf would
//! lift it.
double pinkNoiseGainDb(Effect & effect)
{
    effect.reset();
    std::mt19937 generator { 1234 };
    std::uniform_real_distribution<double> white { -1.0, 1.0 };
    // Paul Kellet's refined filter: within a fraction of a decibel of -3 dB/octave over the band
    std::array<double, 7> b {};
    // A 24 dB/oct Butterworth
    SvfFilter lowCut1;
    SvfFilter lowCut2;
    lowCut1.calculateLowCut(20.0, SampleRate, 0.5412);
    lowCut2.calculateLowCut(20.0, SampleRate, 1.3066);
    double inputPower = 0.0;
    double outputPower = 0.0;
    const auto frames = static_cast<size_t>(SampleRate * 4.0);
    for (size_t i = 0; i < frames; i++) {
        const double w = white(generator);
        b[0] = 0.99886 * b[0] + w * 0.0555179;
        b[1] = 0.99332 * b[1] + w * 0.0750759;
        b[2] = 0.96900 * b[2] + w * 0.1538520;
        b[3] = 0.86650 * b[3] + w * 0.3104856;
        b[4] = 0.55000 * b[4] + w * 0.5329522;
        b[5] = -0.7616 * b[5] - w * 0.0168980;
        const double pink = lowCut2.process(lowCut1.process((b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362) * 0.1));
        b[6] = w * 0.115926;
        double left = pink;
        double right = pink;
        effect.process(left, right);
        if (i >= frames / 4) {
            inputPower += pink * pink;
            outputPower += left * left;
        }
    }
    return 10.0 * std::log10(outputPower / inputPower);
}

//! Whether @p toggled, with auto gain switched on and back off, comes out bit for bit as
//! @p reference, which never had it. Both are set up alike by the caller.
template<typename Eq>
void verifyAutoGainOffLeavesOutputUnchanged(Eq & reference, Eq & toggled)
{
    const auto run = [&](int frames, bool compare) {
        for (int i = 0; i < frames; i++) {
            const double input = std::sin(static_cast<double>(i) * 0.05);
            double referenceLeft = input;
            double referenceRight = -input * 0.5;
            double toggledLeft = referenceLeft;
            double toggledRight = referenceRight;
            reference.process(referenceLeft, referenceRight);
            toggled.process(toggledLeft, toggledRight);
            if (compare) {
                QVERIFY(toggledLeft == referenceLeft);
                QVERIFY(toggledRight == referenceRight);
            }
        }
    };
    setParameter(toggled, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    run(4410, false);
    setParameter(toggled, Constants::NahdXml::xmlKeyAutoGain(), 0.0f);
    QCOMPARE(toggled.autoGainDb(), 0.0);
    // Past the ramp back
    run(882, false);
    run(4410, true);
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

void SimpleEqTest::test_autoGain_engaged_shouldLevelPinkNoise()
{
    auto effect = makeEq();
    setAmount(effect, 1.0f);
    const double uncompensated = pinkNoiseGainDb(effect);
    QVERIFY2(uncompensated > 1.0, qPrintable(QString::number(uncompensated)));

    setParameter(effect, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    QVERIFY(effect.autoGainDb() < -1.0);
    const double compensated = pinkNoiseGainDb(effect);
    QVERIFY2(std::abs(compensated) < 0.5, qPrintable(QString::number(compensated)));
}

void SimpleEqTest::test_autoGain_off_shouldLeaveOutputUnchanged()
{
    auto reference = makeEq();
    auto toggled = makeEq();
    setAmount(reference, 1.0f);
    setAmount(toggled, 1.0f);
    verifyAutoGainOffLeavesOutputUnchanged(reference, toggled);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SimpleEqTest)
