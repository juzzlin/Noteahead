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

#include "doubler_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../common/parameter_mapper.hpp"
#include "../../doubler.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <random>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;
constexpr int frameCount = 48000;
constexpr int settleFrames = 8192;

constexpr float monoSafe = 0.0f;
constexpr float haas = 1.0f;

void setParameter(Doubler & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
}

std::unique_ptr<Doubler> makeEffect(float mode, float amount, float lowCut, float drift = 0.0f, float time = 0.5f)
{
    auto effect = std::make_unique<Doubler>();
    effect->setSampleRate(sampleRate);
    setParameter(*effect, Constants::NahdXml::xmlKeyMode(), mode);
    setParameter(*effect, Constants::NahdXml::xmlKeyDelayTime(), time);
    setParameter(*effect, Constants::NahdXml::xmlKeyAmount(), amount);
    setParameter(*effect, Constants::NahdXml::xmlKeyHpfCutoff(), lowCut);
    setParameter(*effect, Constants::NahdXml::xmlKeyDepth(), drift);
    effect->sync();
    return effect;
}

double sine(int index, double frequency)
{
    return std::sin(2.0 * std::numbers::pi * frequency * index / sampleRate) * 0.5;
}

//! Side energy in the output, for a mono tone in.
double sideEnergyForTone(Doubler & effect, double frequency)
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

void DoublerTest::test_monoSum_monoSafe_shouldReturnTheInputMid()
{
    // What Mono Safe is named for: whatever it is set to, drift included, the copy cancels in the
    // mono sum and a mono listener hears exactly what came in.
    for (const float amount : { 0.0f, 0.5f, 1.0f }) {
        for (const float drift : { 0.0f, 1.0f }) {
            const auto effect = makeEffect(monoSafe, amount, 0.5f, drift);

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

            QVERIFY2(worst < 1.0e-9, qPrintable(QString { "amount %1 drift %2: %3" }.arg(amount).arg(drift).arg(worst)));
        }
    }
}

void DoublerTest::test_amount_zero_shouldLeaveTheSignalAlone()
{
    for (const float mode : { monoSafe, haas }) {
        const auto effect = makeEffect(mode, 0.0f, 0.5f, 1.0f);

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

        QVERIFY2(worst < 1.0e-9, qPrintable(QString { "mode %1: %2" }.arg(mode).arg(worst)));
    }
}

void DoublerTest::test_amount_raised_monoInput_shouldBuildSide()
{
    // The point of the effect: a source with no width at all comes out with some, in either mode.
    // 1030 Hz rather than a round number, so the default delay is not a whole number of its cycles.
    for (const float mode : { monoSafe, haas }) {
        const auto effect = makeEffect(mode, 1.0f, 0.5f);
        QVERIFY2(sideEnergyForTone(*effect, 1030.0) > 1.0, qPrintable(QString { "mode %1" }.arg(mode)));
    }
}

void DoublerTest::test_haas_impulse_shouldMoveTheRightChannelLate()
{
    // At full Amount the right channel is the copy: the click lands there the set time late, and
    // the left is not touched at all. The Low Cut at its bottom, so the click survives it.
    const auto effect = makeEffect(haas, 1.0f, 0.0f);
    const int expectedFrames = static_cast<int>(std::lround(Doubler::timeMs(0.5) * 0.001 * sampleRate));

    int loudestRight = -1;
    double loudestRightLevel = 0.0;
    double worstLeft = 0.0;
    for (int i = 0; i < 4800; i++) {
        double left = i == 0 ? 1.0 : 0.0;
        double right = left;
        const double leftIn = left;
        effect->process(left, right);
        worstLeft = std::max(worstLeft, std::abs(left - leftIn));
        if (std::abs(right) > loudestRightLevel) {
            loudestRightLevel = std::abs(right);
            loudestRight = i;
        }
    }

    QVERIFY(worstLeft < 1.0e-12);
    QVERIFY2(std::abs(loudestRight - expectedFrames) <= 1, qPrintable(QString { "peak at %1, expected %2" }.arg(loudestRight).arg(expectedFrames)));
}

void DoublerTest::test_lowCut_shouldKeepLowFrequenciesOutOfTheCopy()
{
    // What matters is that a low tone is spread far less than a high one, not an absolute number
    const auto corner = static_cast<float>(ParameterMapper::unmapLogFrequency(500.0, 50.0, 5000.0));
    const auto low = makeEffect(monoSafe, 1.0f, corner);
    const double belowCorner = sideEnergyForTone(*low, 100.0);

    const auto high = makeEffect(monoSafe, 1.0f, corner);
    const double aboveCorner = sideEnergyForTone(*high, 2030.0);

    QVERIFY(aboveCorner > 0.0);
    QVERIFY2(belowCorner < aboveCorner / 20.0, qPrintable(QString { "below %1, above %2" }.arg(belowCorner).arg(aboveCorner)));
}

void DoublerTest::test_lowCut_bottom_shouldBeBypassed()
{
    // The readout says Bypass there, so a tone well under the lowest corner reaches the copy as
    // fully as one far above it
    const auto low = makeEffect(monoSafe, 1.0f, 0.0f);
    const double belowCorner = sideEnergyForTone(*low, 30.0);
    const auto high = makeEffect(monoSafe, 1.0f, 0.0f);
    const double aboveCorner = sideEnergyForTone(*high, 2030.0);
    QVERIFY2(belowCorner > aboveCorner * 0.5, qPrintable(QString { "below %1, above %2" }.arg(belowCorner).arg(aboveCorner)));
}

void DoublerTest::test_lowCut_default_sawNotes_shouldStayCentred()
{
    // The copy makes each harmonic louder on one side by where it falls in the comb, which moves
    // with the note. A lead whose fundamental was copied threw whole notes to one side. At the
    // default Low Cut no note of a saw lead's range may lean far either way.
    for (const float mode : { monoSafe, haas }) {
        auto defaults = std::make_unique<Doubler>();
        const auto lowCut = defaults->parameter(Constants::NahdXml::xmlKeyHpfCutoff().toStdString())->get().value();
        for (int note = 53; note <= 77; note++) {
            const auto effect = makeEffect(mode, 0.5f, lowCut, 0.0f, 0.716f);
            const double fundamental = 440.0 * std::pow(2.0, (note - 69) / 12.0);
            double leftEnergy = 0.0;
            double rightEnergy = 0.0;
            for (int i = 0; i < 12000; i++) {
                double saw = 0.0;
                for (int harmonic = 1; harmonic * fundamental < 8000.0; harmonic++) {
                    saw += std::sin(2.0 * std::numbers::pi * harmonic * fundamental * i / sampleRate) / harmonic;
                }
                double left = saw * 0.2;
                double right = left;
                effect->process(left, right);
                if (i >= 4800) {
                    leftEnergy += left * left;
                    rightEnergy += right * right;
                }
            }
            const double balanceDb = 10.0 * std::log10(leftEnergy / rightEnergy);
            QVERIFY2(std::abs(balanceDb) < 2.5, qPrintable(QString { "mode %1 note %2: %3 dB" }.arg(mode).arg(note).arg(balanceDb)));
        }
    }
}

void DoublerTest::test_isSettled_afterSilence_shouldSettle()
{
    // The copy is still on its way out after the input stops, so the device must keep running
    // until the line has emptied, and no longer.
    const auto effect = makeEffect(monoSafe, 1.0f, 0.5f);
    QVERIFY(effect->isSettled());

    double left = 0.5;
    double right = 0.5;
    effect->process(left, right);
    QVERIFY(!effect->isSettled());

    int frames = 0;
    while (!effect->isSettled() && frames < 48000) {
        left = 0.0;
        right = 0.0;
        effect->process(left, right);
        frames++;
    }
    QVERIFY(effect->isSettled());
    // The longest delay plus its margin, and the Low Cut's ringing dying away under the threshold
    QVERIFY2(frames < static_cast<int>(0.2 * sampleRate), qPrintable(QString { "%1 frames" }.arg(frames)));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DoublerTest)
