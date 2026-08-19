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

#include "early_reflections_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/early_reflections.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <vector>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;

//! Half a second, which is well past anything this effect is allowed to still be doing.
constexpr int responseFrames = 24000;

void setParameter(EarlyReflections & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
}

struct Response
{
    std::vector<double> left;
    std::vector<double> right;
};

//! The reflections alone, with the source they were made from taken back out.
//!
//! Mix is additive, so the output is the dry plus the reflections; Solo hands back the difference
//! between that and the dry, which is exactly the reflections.
Response impulseResponse(EarlyReflections & effect)
{
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeySolo(), 1.0f);
    effect.sync();

    Response response;
    response.left.reserve(responseFrames);
    response.right.reserve(responseFrames);

    for (int i = 0; i < responseFrames; i++) {
        double left = i == 0 ? 1.0 : 0.0;
        double right = left;
        effect.process(left, right);
        response.left.push_back(left);
        response.right.push_back(right);
    }

    return response;
}

std::unique_ptr<EarlyReflections> makeEffect(float size, float preDelay, float damping, float diffusion = 0.0f)
{
    auto effect = std::make_unique<EarlyReflections>();
    effect->setSampleRate(sampleRate);
    setParameter(*effect, Constants::NahdXml::xmlKeySize(), size);
    setParameter(*effect, Constants::NahdXml::xmlKeyPreDelay(), preDelay);
    setParameter(*effect, Constants::NahdXml::xmlKeyDamping(), damping);
    // Off unless a test asks for it: the all-passes ring on past the taps, which would hide the tap
    // pattern the geometry tests are actually measuring.
    setParameter(*effect, Constants::NahdXml::xmlKeyDiffusion(), diffusion);
    effect->sync();
    return effect;
}

int firstIndexAbove(const std::vector<double> & signal, double threshold)
{
    for (size_t i = 0; i < signal.size(); i++) {
        if (std::abs(signal[i]) > threshold) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int lastIndexAbove(const std::vector<double> & signal, double threshold)
{
    for (int i = static_cast<int>(signal.size()) - 1; i >= 0; i--) {
        if (std::abs(signal[static_cast<size_t>(i)]) > threshold) {
            return i;
        }
    }
    return -1;
}

double peakBetween(const std::vector<double> & signal, int from, int to)
{
    double peak = 0.0;
    for (int i = std::max(from, 0); i < std::min(to, static_cast<int>(signal.size())); i++) {
        peak = std::max(peak, std::abs(signal[static_cast<size_t>(i)]));
    }
    return peak;
}

//! How much of a stretch of signal is moving quickly, as a share of how much of it there is at
//! all. Normalised, so it compares the colour of two reflections rather than their levels.
double brightness(const std::vector<double> & signal, int from, int to)
{
    double fast = 0.0;
    double total = 0.0;
    for (int i = std::max(from, 1); i < std::min(to, static_cast<int>(signal.size())); i++) {
        const double difference = signal[static_cast<size_t>(i)] - signal[static_cast<size_t>(i - 1)];
        fast += difference * difference;
        total += signal[static_cast<size_t>(i)] * signal[static_cast<size_t>(i)];
    }
    return total > 0.0 ? fast / total : 0.0;
}

} // namespace

void EarlyReflectionsTest::test_mix_zero_shouldLeaveTheSignalAlone()
{
    const auto effect = makeEffect(0.4f, 0.12f, 0.4f);

    std::mt19937 engine { 31337 };
    std::uniform_real_distribution<double> distribution { -0.5, 0.5 };

    double worst = 0.0;
    for (int i = 0; i < 48000; i++) {
        double left = distribution(engine);
        double right = distribution(engine);
        const double leftIn = left;
        const double rightIn = right;
        effect->process(left, right);
        worst = std::max({ worst, std::abs(left - leftIn), std::abs(right - rightIn) });
    }

    // A room nobody has dialled in yet must be inaudible.
    QVERIFY(worst < 1.0e-12);
}

void EarlyReflectionsTest::test_reflections_beforeThePreDelay_shouldBeSilent()
{
    // 20 ms of pre-delay, so nothing may come back for at least that long.
    const auto effect = makeEffect(0.4f, 0.2f, 0.0f);
    const auto response = impulseResponse(*effect);

    const int preDelaySamples = static_cast<int>(0.020 * sampleRate);
    QVERIFY(peakBetween(response.left, 0, preDelaySamples) < 1.0e-9);

    // And something must, or there is no room at all.
    QVERIFY(peakBetween(response.left, preDelaySamples, responseFrames) > 0.01);
}

void EarlyReflectionsTest::test_preDelay_raised_shouldPushTheFirstReflectionLater()
{
    const auto near = makeEffect(0.4f, 0.0f, 0.0f);
    const auto far = makeEffect(0.4f, 0.5f, 0.0f);

    const int nearFirst = firstIndexAbove(impulseResponse(*near).left, 1.0e-6);
    const int farFirst = firstIndexAbove(impulseResponse(*far).left, 1.0e-6);

    QVERIFY(nearFirst >= 0);
    QVERIFY(farFirst > nearFirst);

    // Half of the hundred millisecond range, give or take the first tap's own offset.
    QVERIFY(std::abs((farFirst - nearFirst) - static_cast<int>(0.050 * sampleRate)) < 200);
}

void EarlyReflectionsTest::test_size_raised_shouldSpreadTheReflectionsFurther()
{
    const auto small = makeEffect(0.0f, 0.0f, 0.0f);
    const auto large = makeEffect(1.0f, 0.0f, 0.0f);

    const int smallLast = lastIndexAbove(impulseResponse(*small).left, 1.0e-4);
    const int largeLast = lastIndexAbove(impulseResponse(*large).left, 1.0e-4);

    QVERIFY(smallLast > 0);
    QVERIFY(largeLast > smallLast * 2);
}

void EarlyReflectionsTest::test_reflections_shouldDifferBetweenTheChannels()
{
    // The two channels are given different arrival patterns, which is where the stereo image comes
    // from: a mono source in must come back as something with width.
    const auto effect = makeEffect(0.4f, 0.12f, 0.0f);
    const auto response = impulseResponse(*effect);

    double difference = 0.0;
    double total = 0.0;
    for (size_t i = 0; i < response.left.size(); i++) {
        const double side = response.left[i] - response.right[i];
        difference += side * side;
        total += response.left[i] * response.left[i] + response.right[i] * response.right[i];
    }

    QVERIFY(total > 0.0);
    QVERIFY(difference / total > 0.5);
}

void EarlyReflectionsTest::test_damping_raised_shouldTakeTheTopOffTheReflections()
{
    const auto bright = makeEffect(0.4f, 0.0f, 0.0f);
    const auto dull = makeEffect(0.4f, 0.0f, 1.0f);

    const auto brightResponse = impulseResponse(*bright);
    const auto dullResponse = impulseResponse(*dull);

    // Energy above roughly a quarter of Nyquist, estimated by how much the signal changes from one
    // sample to the next: a dull response moves far less.
    const auto highEnergy = [](const std::vector<double> & signal) {
        double energy = 0.0;
        for (size_t i = 1; i < signal.size(); i++) {
            const double difference = signal[i] - signal[i - 1];
            energy += difference * difference;
        }
        return energy;
    };

    QVERIFY(highEnergy(dullResponse.left) < highEnergy(brightResponse.left) * 0.5);
}

void EarlyReflectionsTest::test_damping_shouldDarkenLateReflectionsMoreThanEarlyOnes()
{
    // Nothing feeds back here, so a single filter on the sum would darken every reflection by the
    // same amount. A room does not: a reflection arriving late has bounced off more surfaces and
    // pushed through more air, and losing the top with distance is one of the cues this effect
    // exists to provide.
    //
    // Measured as the late-to-early colour ratio at two damping settings rather than as an
    // absolute figure. The low cut rings on well past the last tap, and that ringing is
    // low-frequency whatever the damping is set to; comparing two runs through the same windows
    // cancels it out of both.
    const auto open = makeEffect(0.4f, 0.0f, 0.0f);
    const auto damped = makeEffect(0.4f, 0.0f, 1.0f);

    const auto openResponse = impulseResponse(*open);
    const auto dampedResponse = impulseResponse(*damped);

    const double peak = peakBetween(openResponse.left, 0, responseFrames);
    QVERIFY(peak > 0.0);

    // The cloud runs from the first reflection to the last one still worth the name.
    const int first = firstIndexAbove(openResponse.left, peak * 0.05);
    const int last = lastIndexAbove(openResponse.left, peak * 0.05);
    QVERIFY(first >= 0);
    QVERIFY(last > first + 400);

    const int quarter = (last - first) / 4;
    const auto colourRatio = [&](const std::vector<double> & response) {
        const double early = brightness(response, first, first + quarter);
        const double late = brightness(response, last - quarter, last);
        return early > 0.0 ? late / early : 0.0;
    };

    const double openRatio = colourRatio(openResponse.left);
    const double dampedRatio = colourRatio(dampedResponse.left);

    QVERIFY2(dampedRatio < openRatio * 0.5,
             qPrintable(QString { "open %1, damped %2" }.arg(openRatio).arg(dampedRatio)));
}

void EarlyReflectionsTest::test_reflections_shouldStopRatherThanTail()
{
    // Nothing feeds back anywhere, so once the longest tap has been and gone there is nothing left.
    // That is the whole difference between this and a reverb.
    const auto effect = makeEffect(1.0f, 1.0f, 0.0f);
    const auto response = impulseResponse(*effect);

    // Longest tap: 100 ms of pre-delay plus a 140 ms span, and 40 ms for the filters to settle.
    const int settled = static_cast<int>(0.28 * sampleRate);
    QVERIFY(peakBetween(response.left, 0, settled) > 0.01);
    QVERIFY(peakBetween(response.left, settled, responseFrames) < 1.0e-4);
}

void EarlyReflectionsTest::test_diffusion_raised_shouldFillTheGapsBetweenTaps()
{
    // Sixteen discrete arrivals is a fraction of what a room returns, and sparse taps read as an
    // effect rather than a space. Diffusion is what turns them into something continuous, so the
    // measure is how much of the response is carrying anything at all.
    const auto sparse = makeEffect(0.4f, 0.0f, 0.0f, 0.0f);
    const auto dense = makeEffect(0.4f, 0.0f, 0.0f, 1.0f);

    const auto sparseResponse = impulseResponse(*sparse);
    const auto denseResponse = impulseResponse(*dense);

    const auto density = [](const std::vector<double> & response) {
        const double peak = peakBetween(response, 0, responseFrames);
        int carrying = 0;
        for (const double value : response) {
            if (std::abs(value) > peak * 0.001) {
                carrying++;
            }
        }
        return carrying;
    };

    const int sparseCount = density(sparseResponse.left);
    const int denseCount = density(denseResponse.left);

    QVERIFY2(denseCount > sparseCount * 4,
             qPrintable(QString { "sparse %1 samples, diffused %2" }.arg(sparseCount).arg(denseCount)));
}

void EarlyReflectionsTest::test_diffusion_raised_shouldStillDecay()
{
    // A Schroeder all-pass is recursive, so diffusion does bring feedback with it and the response
    // no longer stops dead at the last tap. It must still be over quickly: this is a room, and the
    // moment it rings on it is a reverb competing with the ones that do it properly.
    const auto effect = makeEffect(1.0f, 0.0f, 0.0f, 1.0f);
    const auto response = impulseResponse(*effect);

    const int halfSecond = static_cast<int>(0.5 * sampleRate);
    QVERIFY(peakBetween(response.left, 0, halfSecond) > 0.01);
    QVERIFY2(peakBetween(response.left, halfSecond, responseFrames) < 1.0e-3,
             qPrintable(QString { "still ringing at half a second: %1" }.arg(peakBetween(response.left, halfSecond, responseFrames))));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EarlyReflectionsTest)
