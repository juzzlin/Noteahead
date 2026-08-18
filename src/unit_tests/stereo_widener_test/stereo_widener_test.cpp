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

#include "stereo_widener_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/stereo_widener.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <random>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;

//! Long enough that the group delay of the crossovers is negligible against the window, and that
//! the correlation meters have settled several times over.
constexpr int frameCount = 96000;

//! The crossovers start from rest, so the first few milliseconds are not representative.
constexpr int settleFrames = 4800;

void setParameter(StereoWidener & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
}

//! Every band at the same width, which is the state most of these tests want: what is being checked
//! is what width does, not what the split does.
std::unique_ptr<StereoWidener> makeEffect(double width)
{
    auto effect = std::make_unique<StereoWidener>();
    effect->setSampleRate(sampleRate);

    // Half the control's travel is 100 %, so the internal value is half the width asked for.
    for (size_t i = 0; i < StereoWidener::NumBands; i++) {
        setParameter(*effect, Constants::NahdXml::xmlKeyBandWidth(i), static_cast<float>(width * 0.5));
    }
    effect->sync();

    return effect;
}

//! Two channels of independent noise, which is the worst case every width control has to survive.
struct NoiseSource
{
    std::mt19937 engine { 20260818 };
    std::uniform_real_distribution<double> distribution { -0.5, 0.5 };

    void next(double & left, double & right)
    {
        left = distribution(engine);
        right = distribution(engine);
    }
};

double rootMeanSquare(double sumOfSquares, int count)
{
    return count > 0 ? std::sqrt(sumOfSquares / count) : 0.0;
}

} // namespace

void StereoWidenerTest::test_width_default_shouldLeaveSignalAlone()
{
    // Built and used without touching a control, which is what dropping one into a rack gives you.
    StereoWidener effect;
    effect.setSampleRate(sampleRate);
    NoiseSource noise;

    double midIn = 0.0;
    double midOut = 0.0;
    double sideIn = 0.0;
    double sideOut = 0.0;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        noise.next(left, right);
        const double midBefore = (left + right) * 0.5;
        const double sideBefore = (left - right) * 0.5;
        effect.process(left, right);
        if (i >= settleFrames) {
            midIn += midBefore * midBefore;
            midOut += (left + right) * (left + right) * 0.25;
            sideIn += sideBefore * sideBefore;
            sideOut += (left - right) * (left - right) * 0.25;
        }
    }

    QVERIFY(midIn > 0.0);
    QVERIFY(sideIn > 0.0);

    // Every band starts at 100 %, which is the setting that does nothing at all.
    QVERIFY(std::abs(midOut / midIn - 1.0) < 0.05);
    QVERIFY(std::abs(sideOut / sideIn - 1.0) < 0.05);
}

void StereoWidenerTest::test_width_unity_shouldPreserveMidEnergy()
{
    const auto effect = makeEffect(1.0);
    NoiseSource noise;

    double midIn = 0.0;
    double midOut = 0.0;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        noise.next(left, right);
        const double before = (left + right) * 0.5;
        effect->process(left, right);
        const double after = (left + right) * 0.5;
        if (i >= settleFrames) {
            midIn += before * before;
            midOut += after * after;
        }
    }

    QVERIFY(midIn > 0.0);

    // 100 % has to leave the centre exactly where the side signal was left: alone.
    QVERIFY(std::abs(midOut / midIn - 1.0) < 0.05);
}

void StereoWidenerTest::test_width_unity_shouldPreserveSideEnergy()
{
    const auto effect = makeEffect(1.0);
    NoiseSource noise;

    double sideIn = 0.0;
    double sideOut = 0.0;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        noise.next(left, right);
        const double before = (left - right) * 0.5;
        effect->process(left, right);
        const double after = (left - right) * 0.5;
        if (i >= settleFrames) {
            sideIn += before * before;
            sideOut += after * after;
        }
    }

    QVERIFY(sideIn > 0.0);

    // The three bands sum back to an all-pass of what went in, which moves phase around but not
    // energy, so at 100 % the side signal has to come out at the level it arrived.
    QVERIFY(std::abs(sideOut / sideIn - 1.0) < 0.05);
}

void StereoWidenerTest::test_width_zero_shouldSumToMono()
{
    const auto effect = makeEffect(0.0);
    NoiseSource noise;

    double largestDifference = 0.0;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        noise.next(left, right);
        effect->process(left, right);
        if (i >= settleFrames) {
            largestDifference = std::max(largestDifference, std::abs(left - right));
        }
    }

    // Every band has had its side signal taken to nothing, so the two channels are the same signal.
    QVERIFY(largestDifference < 1.0e-12);
}

void StereoWidenerTest::test_width_doubled_shouldDoubleSideAmplitude()
{
    const auto effect = makeEffect(2.0);
    NoiseSource noise;

    double sideIn = 0.0;
    double sideOut = 0.0;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        noise.next(left, right);
        const double before = (left - right) * 0.5;
        effect->process(left, right);
        const double after = (left - right) * 0.5;
        if (i >= settleFrames) {
            sideIn += before * before;
            sideOut += after * after;
        }
    }

    QVERIFY(sideIn > 0.0);

    // Twice the amplitude is four times the energy.
    QVERIFY(std::abs(sideOut / sideIn - 4.0) < 0.2);
}

void StereoWidenerTest::test_width_zero_shouldPreserveMidEnergy()
{
    const auto effect = makeEffect(0.0);
    NoiseSource noise;

    double midIn = 0.0;
    double midOut = 0.0;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        noise.next(left, right);
        const double before = (left + right) * 0.5;
        effect->process(left, right);
        const double after = (left + right) * 0.5;
        if (i >= settleFrames) {
            midIn += before * before;
            midOut += after * after;
        }
    }

    QVERIFY(midIn > 0.0);

    // Narrowing works on the side signal alone: what was in the centre has to still be there.
    QVERIFY(std::abs(midOut / midIn - 1.0) < 0.05);
}

void StereoWidenerTest::test_monoBass_enabled_shouldCentreLowFrequencies()
{
    const auto effect = makeEffect(1.0);
    setParameter(*effect, Constants::NahdXml::xmlKeyMonoBass(), 1.0f);
    effect->sync();

    double difference = 0.0;
    double sum = 0.0;
    int counted = 0;

    // Well below the default 120 Hz corner, and in one channel only, so there is as much to centre
    // as there can be.
    for (int i = 0; i < frameCount; i++) {
        double left = std::sin(2.0 * std::numbers::pi * 50.0 * i / sampleRate);
        double right = 0.0;
        effect->process(left, right);
        if (i >= settleFrames) {
            difference += (left - right) * (left - right);
            sum += (left + right) * (left + right);
            counted++;
        }
    }

    QVERIFY(sum > 0.0);

    // What is left off-centre is only what leaked through the crossover's high path.
    QVERIFY(rootMeanSquare(difference, counted) / rootMeanSquare(sum, counted) < 0.15);
}

void StereoWidenerTest::test_monoBass_disabled_shouldLeaveLowFrequenciesAlone()
{
    const auto effect = makeEffect(1.0);

    double difference = 0.0;
    double sum = 0.0;
    int counted = 0;

    for (int i = 0; i < frameCount; i++) {
        double left = std::sin(2.0 * std::numbers::pi * 50.0 * i / sampleRate);
        double right = 0.0;
        effect->process(left, right);
        if (i >= settleFrames) {
            difference += (left - right) * (left - right);
            sum += (left + right) * (left + right);
            counted++;
        }
    }

    QVERIFY(sum > 0.0);

    // Nothing centred it, so a signal in one channel is still entirely off-centre.
    QVERIFY(rootMeanSquare(difference, counted) / rootMeanSquare(sum, counted) > 0.8);
}

void StereoWidenerTest::test_monoBass_enabled_shouldLeaveHighFrequenciesAlone()
{
    const auto effect = makeEffect(1.0);
    setParameter(*effect, Constants::NahdXml::xmlKeyMonoBass(), 1.0f);
    effect->sync();

    double difference = 0.0;
    double sum = 0.0;
    int counted = 0;

    for (int i = 0; i < frameCount; i++) {
        double left = std::sin(2.0 * std::numbers::pi * 5000.0 * i / sampleRate);
        double right = 0.0;
        effect->process(left, right);
        if (i >= settleFrames) {
            difference += (left - right) * (left - right);
            sum += (left + right) * (left + right);
            counted++;
        }
    }

    QVERIFY(sum > 0.0);

    // Far above the corner, so the stage has to be as good as absent.
    QVERIFY(rootMeanSquare(difference, counted) / rootMeanSquare(sum, counted) > 0.8);
}

void StereoWidenerTest::test_bandSolo_enabled_shouldPassOnlySoloedBand()
{
    const auto effect = makeEffect(1.0);
    setParameter(*effect, Constants::NahdXml::xmlKeyBandSolo(2), 1.0f);
    effect->sync();

    double input = 0.0;
    double output = 0.0;
    int counted = 0;

    // A tone two crossovers below the soloed band.
    for (int i = 0; i < frameCount; i++) {
        double left = std::sin(2.0 * std::numbers::pi * 50.0 * i / sampleRate);
        double right = left;
        const double before = left;
        effect->process(left, right);
        if (i >= settleFrames) {
            input += before * before;
            output += left * left;
            counted++;
        }
    }

    QVERIFY(input > 0.0);
    QVERIFY(rootMeanSquare(output, counted) / rootMeanSquare(input, counted) < 0.05);
}

void StereoWidenerTest::test_bandCorrelation_monoInput_shouldReadCentred()
{
    const auto effect = makeEffect(1.0);
    NoiseSource noise;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double unused = 0.0;
        noise.next(left, unused);
        double right = left;
        effect->process(left, right);
    }

    for (size_t band = 0; band < StereoWidener::NumBands; band++) {
        QVERIFY(effect->bandCorrelation(band) > 0.95f);
    }
}

void StereoWidenerTest::test_bandCorrelation_widenedInput_shouldReadBelowCentred()
{
    const auto effect = makeEffect(2.0);
    NoiseSource noise;

    for (int i = 0; i < frameCount; i++) {
        double left = 0.0;
        double right = 0.0;
        noise.next(left, right);
        effect->process(left, right);
    }

    for (size_t band = 0; band < StereoWidener::NumBands; band++) {
        QVERIFY(effect->bandCorrelation(band) < 0.5f);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StereoWidenerTest)
