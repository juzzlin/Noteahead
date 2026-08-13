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

#include "compressor_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../domain/effects/compressor.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

constexpr double sampleRate = 44100.0;

//! Threshold parameter value that lands on exactly -20dB within the -60..0dB range. The default
//! sits at -20.4dB, which is too coarse to place a signal exactly on the knee.
constexpr float thresholdMinus20Db = 40.0f / 60.0f;

void setParameter(Compressor & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().setValue(value);
    }
    effect.sync();
}

//! Feeds a steady stereo level for the given number of frames and returns the last output frame.
double feed(Compressor & effect, double level, int frameCount)
{
    double left = 0.0;
    double right = 0.0;
    for (int i = 0; i < frameCount; i++) {
        left = level;
        right = level;
        effect.process(left, right);
    }
    return left;
}

} // namespace

void CompressorTest::test_gain_belowThreshold_shouldPassThroughUnchanged()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    // Defaults: threshold -20dB, ratio 4:1, attack 10ms, release 100ms, makeup 0dB, lookahead 0ms.
    const double level = Utils::Dsp::dbToLinear(-30.0);
    double left = level;
    double right = level;
    effect.process(left, right);

    QCOMPARE(left, level);
    QCOMPARE(right, level);
    QCOMPARE(effect.reductionDb(), 0.0f);
}

void CompressorTest::test_gain_aboveThreshold_shouldSettleToRatioReduction()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    // 0dB into a -20dB threshold at 4:1 overshoots by 20dB, so it settles 20 * (1 - 1/4) = 15dB down.
    feed(effect, 1.0, 5000);

    QVERIFY(effect.reductionDb() < -14.0f);
    QVERIFY(effect.reductionDb() > -16.0f);

    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    QVERIFY(left < Utils::Dsp::dbToLinear(-14.0f));
}

void CompressorTest::test_gain_higherRatio_shouldReduceMore()
{
    const auto settledReductionDb = [](float ratioValue) {
        Compressor effect;
        effect.setSampleRate(sampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyRatio(), ratioValue);
        feed(effect, 1.0, 5000);
        return effect.reductionDb();
    };

    // Internal 0 is 1:1, which cannot reduce at all, and the ratio only climbs from there.
    QCOMPARE(settledReductionDb(0.0f), 0.0f);
    QVERIFY(settledReductionDb(1.0f) < settledReductionDb(0.5f));
    QVERIFY(settledReductionDb(0.5f) < 0.0f);
}

void CompressorTest::test_gain_makeup_shouldLiftOutput()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    const double level = Utils::Dsp::dbToLinear(-30.0);

    // Internal 0.75 of a -12..+12dB range is +6dB, and below the threshold it is the only gain applied.
    setParameter(effect, Constants::NahdXml::xmlKeyMakeup(), 0.75f);
    const double output = feed(effect, level, 16);

    QCOMPARE(effect.reductionDb(), 0.0f);
    QVERIFY(std::abs(output - level * Utils::Dsp::dbToLinear(6.0f)) < 1.0e-6);
}

void CompressorTest::test_knee_zero_shouldNotReduceAtThreshold()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    // The default threshold is -20.4dB, so pin it to a round number the signal can sit exactly on.
    setParameter(effect, Constants::NahdXml::xmlKeyThreshold(), thresholdMinus20Db);
    feed(effect, Utils::Dsp::dbToLinear(-20.0), 5000);

    QCOMPARE(effect.reductionDb(), 0.0f);
}

void CompressorTest::test_knee_soft_shouldReduceBelowThreshold()
{
    const auto reductionBelowThresholdDb = [](float kneeValue) {
        Compressor effect;
        effect.setSampleRate(sampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyThreshold(), thresholdMinus20Db);
        setParameter(effect, Constants::NahdXml::xmlKeyKnee(), kneeValue);
        feed(effect, Utils::Dsp::dbToLinear(-24.0), 5000);
        return effect.reductionDb();
    };

    // 4dB under the threshold a hard knee has nothing to do at all.
    QCOMPARE(reductionBelowThresholdDb(0.0f), 0.0f);

    // A 12dB knee (internal 0.5 of 0..24dB) reaches 6dB below the threshold, so it is already
    // bending here, gently: the full-ratio curve would be 3dB down by this point.
    const float softKneeReduction = reductionBelowThresholdDb(0.5f);
    QVERIFY(softKneeReduction < 0.0f);
    QVERIFY(softKneeReduction > -3.0f);
}

void CompressorTest::test_attack_slow_shouldReachReductionLater()
{
    const auto reductionAfter = [](float attackValue, int frameCount) {
        Compressor effect;
        effect.setSampleRate(sampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyAttack(), attackValue);
        feed(effect, 1.0, frameCount);
        return effect.reductionDb();
    };

    const float fastAttack = static_cast<float>(ParameterMapper::unmapExponential(1.0, 0.1, 500.0));
    const float slowAttack = static_cast<float>(ParameterMapper::unmapExponential(200.0, 0.1, 500.0));

    // Half a millisecond in, the 1ms attack is well under way and the 200ms one has barely moved.
    const int frames = static_cast<int>(sampleRate * 0.0005);
    QVERIFY(reductionAfter(fastAttack, frames) < reductionAfter(slowAttack, frames));

    // Given long enough both arrive at the same place.
    QVERIFY(std::abs(reductionAfter(fastAttack, 200000) - reductionAfter(slowAttack, 200000)) < 0.5f);
}

void CompressorTest::test_release_afterSignalStops_shouldReturnTowardsUnity()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    setParameter(effect, Constants::NahdXml::xmlKeyRelease(), static_cast<float>(ParameterMapper::unmapExponential(50.0, 1.0, 2000.0)));
    feed(effect, 1.0, 5000);
    const float compressed = effect.reductionDb();
    QVERIFY(compressed < -14.0f);

    // One release time constant of silence recovers most of the way back, but not all of it.
    feed(effect, 0.0, static_cast<int>(sampleRate * 0.05));
    QVERIFY(effect.reductionDb() > compressed);
    QVERIFY(effect.reductionDb() < 0.0f);

    feed(effect, 0.0, static_cast<int>(sampleRate * 0.5));
    QVERIFY(effect.reductionDb() > -0.1f);
}

void CompressorTest::test_lookahead_shouldDelayOutputButNotDetection()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    setParameter(effect, Constants::NahdXml::xmlKeyLookahead(), 1.0f); // 100% is 10ms

    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);

    // The delay line is still full of the silence it started with.
    QCOMPARE(left, 0.0);
    QCOMPARE(right, 0.0);

    // The detector, however, sees the input as it arrives, which is the point of lookahead.
    QVERIFY(effect.reductionDb() < 0.0f);
}

void CompressorTest::test_detectorMode_default_shouldBePeak()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    const auto mode = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString());
    QVERIFY(mode.has_value());
    QCOMPARE(mode->get().value(), 0.0f);

    // A single full-scale sample is already above the -20dB threshold, so a peak detector must react to it.
    double left = 1.0;
    double right = 1.0;
    effect.process(left, right);
    QVERIFY(effect.reductionDb() < 0.0f);
}

void CompressorTest::test_detectorMode_rms_shouldIgnoreShortTransients()
{
    const auto burstReductionDb = [](Compressor::DetectorMode mode, int frameCount) {
        Compressor effect;
        effect.setSampleRate(sampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyMode(), mode == Compressor::DetectorMode::Rms ? 1.0f : 0.0f);
        feed(effect, 1.0, frameCount);
        return effect.reductionDb();
    };

    // A ~0.5ms burst is far shorter than the RMS window, so the RMS detector must stay closer to unity gain.
    const int burstFrames = 20;
    QVERIFY(burstReductionDb(Compressor::DetectorMode::Rms, burstFrames) > burstReductionDb(Compressor::DetectorMode::Peak, burstFrames));
}

void CompressorTest::test_detectorMode_rms_shouldSettleToSameReduction()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyMode(), 1.0f);

    // On a steady full-scale signal both detectors end up in the same place: 20dB over the
    // threshold at 4:1 means 15dB down.
    feed(effect, 1.0, 20000);

    QVERIFY(effect.reductionDb() < -14.0f);
    QVERIFY(effect.reductionDb() > -16.0f);
}

void CompressorTest::test_reset_afterCompressing_shouldReturnToUnity()
{
    Compressor effect;
    effect.setSampleRate(sampleRate);

    feed(effect, 1.0, 5000);
    QVERIFY(effect.reductionDb() < -14.0f);

    effect.reset();
    QCOMPARE(effect.reductionDb(), 0.0f);

    // The delay line has been cleared too, so a quiet signal passes straight through again.
    const double level = Utils::Dsp::dbToLinear(-30.0);
    double left = level;
    double right = level;
    effect.process(left, right);
    QCOMPARE(left, level);
}

void CompressorTest::test_sidechainSourceDeviceIndex_unset_shouldBeEmpty()
{
    Compressor effect;

    QVERIFY(!effect.sidechainSourceDeviceIndex());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::CompressorTest)
