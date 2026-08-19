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

#include "dimension_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/dimension.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <random>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;

//! Long enough for the shifters' own delay to have filled and for the side to have built.
constexpr int frameCount = 96000;
constexpr int settleFrames = 8192;

void setParameter(Dimension & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
}

std::unique_ptr<Dimension> makeEffect(float detune, float amount, float lowCut)
{
    auto effect = std::make_unique<Dimension>();
    effect->setSampleRate(sampleRate);
    setParameter(*effect, Constants::NahdXml::xmlKeyDetune(), detune);
    setParameter(*effect, Constants::NahdXml::xmlKeyAmount(), amount);
    setParameter(*effect, Constants::NahdXml::xmlKeyHpfCutoff(), lowCut);
    effect->sync();
    return effect;
}

double sine(int index, double frequency)
{
    return std::sin(2.0 * std::numbers::pi * frequency * index / sampleRate) * 0.5;
}

//! Side energy in the output, for a mono tone in.
double sideEnergyForTone(Dimension & effect, double frequency)
{
    double energy = 0.0;
    for (int i = 0; i < frameCount; i++) {
        double left = sine(i, frequency);
        double right = left;
        effect.process(left, right);
        if (i >= settleFrames) {
            const double side = (left - right) * 0.5;
            energy += side * side;
        }
    }
    return energy;
}

} // namespace

void DimensionTest::test_monoSum_anySetting_shouldReturnTheInputMid()
{
    // The claim the whole effect rests on: whatever it is set to, folding the output down to mono
    // returns exactly what was folded down at the input. Nothing a mono listener hears can change.
    for (const float detune : { 0.0f, 0.28f, 1.0f }) {
        for (const float amount : { 0.0f, 0.5f, 1.0f }) {
            const auto effect = makeEffect(detune, amount, 0.5f);

            std::mt19937 engine { 4242 };
            std::uniform_real_distribution<double> distribution { -0.5, 0.5 };

            double worst = 0.0;
            for (int i = 0; i < frameCount; i++) {
                double left = distribution(engine);
                double right = distribution(engine);
                const double sumIn = left + right;
                effect->process(left, right);
                worst = std::max(worst, std::abs((left + right) - sumIn));
            }

            QVERIFY2(worst < 1.0e-9, qPrintable(QString { "detune %1 amount %2: %3" }.arg(detune).arg(amount).arg(worst)));
        }
    }
}

void DimensionTest::test_amount_zero_shouldLeaveTheSignalAlone()
{
    const auto effect = makeEffect(0.28f, 0.0f, 0.5f);

    std::mt19937 engine { 99 };
    std::uniform_real_distribution<double> distribution { -0.5, 0.5 };

    double worst = 0.0;
    for (int i = 0; i < frameCount; i++) {
        double left = distribution(engine);
        double right = distribution(engine);
        const double leftIn = left;
        const double rightIn = right;
        effect->process(left, right);
        worst = std::max({ worst, std::abs(left - leftIn), std::abs(right - rightIn) });
    }

    QVERIFY(worst < 1.0e-9);
}

void DimensionTest::test_amount_raised_monoInput_shouldBuildSide()
{
    // The point of the effect: a source with no width at all comes out with some.
    const auto effect = makeEffect(0.28f, 1.0f, 0.5f);
    QVERIFY(sideEnergyForTone(*effect, 1000.0) > 1.0);
}

void DimensionTest::test_detune_zero_shouldBuildNoSide()
{
    // With the two copies shifted by nothing they are the same signal, and their difference is
    // silence however far Amount is turned up.
    const auto effect = makeEffect(0.0f, 1.0f, 0.5f);
    QVERIFY(sideEnergyForTone(*effect, 1000.0) < 1.0e-9);
}

void DimensionTest::test_lowCut_shouldKeepLowFrequenciesOutOfTheSide()
{
    // Spreading the bottom spends headroom on something the ear cannot place, so what matters is
    // not that a low tone builds nothing but that it builds far less than a high one does. An
    // absolute threshold would only be a restatement of the filter's slope at one frequency.
    const auto low = makeEffect(0.28f, 1.0f, 0.72f); // Corner at about 200 Hz
    const double belowCorner = sideEnergyForTone(*low, 40.0);

    const auto high = makeEffect(0.28f, 1.0f, 0.72f);
    const double aboveCorner = sideEnergyForTone(*high, 2000.0);

    QVERIFY(aboveCorner > 0.0);
    QVERIFY2(belowCorner < aboveCorner / 20.0,
             qPrintable(QString { "below %1, above %2" }.arg(belowCorner).arg(aboveCorner)));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DimensionTest)
