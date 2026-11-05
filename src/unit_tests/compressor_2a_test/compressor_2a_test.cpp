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

#include "compressor_2a_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/opto_compressor_core.hpp"
#include "../../domain/effects/compressor_2a.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

constexpr double rate { 48000.0 };
//! Where devices play.
constexpr double deviceLevel { 0.05 };

void setParameter(Compressor2A & effect, const QString & key, float value)
{
    if (const auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

std::unique_ptr<Compressor2A> makeCompressor(float peakReduction)
{
    auto effect = std::make_unique<Compressor2A>();
    effect->setSampleRate(rate);
    setParameter(*effect, C::xmlKeyPeakReduction(), peakReduction);
    return effect;
}

std::vector<double> sine(double hz, double amplitude, size_t frames)
{
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        stereo[i * 2] = stereo[i * 2 + 1] = amplitude * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(i) / rate);
    }
    return stereo;
}

std::vector<double> process(Compressor2A & effect, std::vector<double> stereo)
{
    const auto frames = static_cast<uint32_t>(stereo.size() / 2);
    AudioContext context { std::span(stereo.data(), stereo.size()), frames, static_cast<uint32_t>(rate) };
    effect.process(context);
    return stereo;
}

double rms(const std::vector<double> & stereo, size_t fromFrame)
{
    double sum = 0.0;
    size_t count = 0;
    for (size_t i = fromFrame; i < stereo.size() / 2; i++) {
        sum += stereo[i * 2] * stereo[i * 2];
        count++;
    }
    return std::sqrt(sum / static_cast<double>(std::max<size_t>(1, count)));
}

double db(double ratio)
{
    return 20.0 * std::log10(ratio);
}

//! Feeds the core a 1 kHz tone for @p seconds and returns the reduction, in positive dB, after it.
double burst(OptoCompressorCore & core, double amplitude, double seconds)
{
    const auto frames = static_cast<size_t>(seconds * rate);
    for (size_t i = 0; i < frames; i++) {
        const double x = amplitude * std::sin(2.0 * std::numbers::pi * 1000.0 * static_cast<double>(i) / rate);
        core.processGainDb(x, x);
    }
    return -core.reductionDb();
}

//! Seconds of silence until the reduction falls to @p fraction of what it was.
double releaseTime(OptoCompressorCore & core, double fraction)
{
    const double start = -core.reductionDb();
    for (size_t i = 0; i < static_cast<size_t>(30.0 * rate); i++) {
        if (-core.processGainDb(0.0, 0.0) <= start * fraction) {
            return static_cast<double>(i) / rate;
        }
    }
    return 30.0;
}

OptoCompressorCore makeCore(double peakReduction, bool limit = false)
{
    OptoCompressorCore core;
    core.setSampleRate(rate);
    core.setPeakReduction(peakReduction);
    core.setLimit(limit);
    return core;
}

} // namespace

void Compressor2ATest::test_peakReduction_zero_shouldPassTheSignal()
{
    auto effect = makeCompressor(0.0f);
    const auto input = sine(1000.0, deviceLevel, 48000);
    const auto output = process(*effect, input);
    const double change = db(rms(output, 24000) / rms(input, 24000));
    QVERIFY2(std::abs(change) < 0.05, qPrintable(QString::number(change)));
    QCOMPARE(effect->reductionDb(), 0.0f);
}

void Compressor2ATest::test_peakReduction_raised_shouldHoldTheLevelDown()
{
    // The middle of the travel holds a device's level down by several dB
    auto effect = makeCompressor(0.5f);
    const auto input = sine(1000.0, deviceLevel, 96000);
    const auto output = process(*effect, input);
    const double change = db(rms(output, 72000) / rms(input, 72000));
    QVERIFY2(change < -4.0 && change > -12.0, qPrintable(QString::number(change)));
    QVERIFY(effect->reductionDb() < -4.0f);
}

void Compressor2ATest::test_staticCurve_limit_shouldReduceMoreThanCompress()
{
    const auto compress = makeCore(0.6, false);
    const auto limit = makeCore(0.6, true);
    for (const double levelDb : { -20.0, -10.0, 0.0 }) {
        QVERIFY2(limit.targetReductionDb(levelDb) > compress.targetReductionDb(levelDb), qPrintable(QString::number(levelDb)));
    }
    // Well above the knee the slopes are those of 3:1 and 10:1
    const double compressSlope = (compress.targetReductionDb(0.0) - compress.targetReductionDb(-10.0)) / 10.0;
    const double limitSlope = (limit.targetReductionDb(0.0) - limit.targetReductionDb(-10.0)) / 10.0;
    QVERIFY2(std::abs(compressSlope - 2.0 / 3.0) < 0.01, qPrintable(QString::number(compressSlope)));
    QVERIFY2(std::abs(limitSlope - 0.9) < 0.01, qPrintable(QString::number(limitSlope)));
}

void Compressor2ATest::test_staticCurve_knee_shouldBeSoft()
{
    // No corner: the slope grows in small steps through the knee
    const auto core = makeCore(0.5);
    double previousSlope = 0.0;
    for (double level = -60.0; level < 0.0; level += 0.5) {
        const double slope = (core.targetReductionDb(level + 0.5) - core.targetReductionDb(level)) / 0.5;
        QVERIFY2(slope >= previousSlope - 1.0e-9, qPrintable(QString::number(level)));
        QVERIFY2(slope - previousSlope < 0.05, qPrintable(QString::number(level)));
        previousSlope = slope;
    }
}

void Compressor2ATest::test_attack_shouldPullDownWithinTensOfMilliseconds()
{
    auto core = makeCore(0.6);
    const double target = core.targetReductionDb(db(deviceLevel));
    QVERIFY(target > 6.0);
    const double early = burst(core, deviceLevel, 0.002);
    const double after = burst(core, deviceLevel, 0.028);
    QVERIFY2(early < target * 0.4, qPrintable(QString::number(early)));
    QVERIFY2(after > target * 0.85, qPrintable(QString::number(after)));
}

void Compressor2ATest::test_release_shouldLetHalfGoWithinSixtyMilliseconds()
{
    for (const double seconds : { 0.2, 1.0, 5.0 }) {
        auto core = makeCore(0.6);
        burst(core, deviceLevel, seconds);
        const double half = releaseTime(core, 0.5);
        QVERIFY2(half > 0.015 && half < 0.09, qPrintable(QString { "%1 s after %2 s" }.arg(half).arg(seconds)));
    }
}

void Compressor2ATest::test_release_longBurst_shouldLetGoSlower()
{
    // Back within 1 dB: at once after a short peak, over seconds after a long loud passage
    const auto recovery = [](double seconds) {
        auto core = makeCore(0.6);
        const double held = burst(core, deviceLevel, seconds);
        return releaseTime(core, 1.0 / held);
    };
    const double shortBurst = recovery(0.2);
    const double longBurst = recovery(5.0);
    QVERIFY2(shortBurst < 0.2, qPrintable(QString::number(shortBurst)));
    QVERIFY2(longBurst > 1.0 && longBurst < 15.0, qPrintable(QString::number(longBurst)));
}

void Compressor2ATest::test_gain_shouldMoveTheOutput()
{
    const auto level = [](double gainDb) {
        auto effect = makeCompressor(0.0f);
        setParameter(*effect, C::xmlKeyGain(), Compressor2A::gainPosition(gainDb));
        return rms(process(*effect, sine(1000.0, deviceLevel, 48000)), 24000);
    };
    const double change = db(level(10.0) / level(0.0));
    QVERIFY2(std::abs(change - 10.0) < 0.1, qPrintable(QString::number(change)));
}

void Compressor2ATest::test_valve_shouldAddASecondHarmonic()
{
    // Measured on the 2 kHz bin of a 1 kHz tone, against the fundamental
    const auto harmonic = [](double amplitude) {
        auto effect = makeCompressor(0.0f);
        const auto output = process(*effect, sine(1000.0, amplitude, 48000));
        double re1 = 0.0, im1 = 0.0, re2 = 0.0, im2 = 0.0;
        for (size_t i = 24000; i < 48000; i++) {
            const double phase = 2.0 * std::numbers::pi * 1000.0 * static_cast<double>(i) / rate;
            re1 += output[i * 2] * std::cos(phase);
            im1 += output[i * 2] * std::sin(phase);
            re2 += output[i * 2] * std::cos(2.0 * phase);
            im2 += output[i * 2] * std::sin(2.0 * phase);
        }
        return db(std::hypot(re2, im2) / std::hypot(re1, im1));
    };
    const double quiet = harmonic(deviceLevel);
    const double hot = harmonic(0.5);
    QVERIFY2(quiet < -40.0 && quiet > -60.0, qPrintable(QString::number(quiet)));
    QVERIFY2(hot > quiet + 15.0, qPrintable(QString::number(hot)));
}

void Compressor2ATest::test_mix_zero_shouldLeaveTheSignalAlone()
{
    auto effect = makeCompressor(0.8f);
    setParameter(*effect, C::xmlKeyMix(), 0.0f);
    const auto input = sine(1000.0, deviceLevel, 4800);
    QCOMPARE(process(*effect, input), input);
}

void Compressor2ATest::test_isSettled_afterSilence_shouldSettle()
{
    auto effect = makeCompressor(0.6f);
    process(*effect, sine(1000.0, deviceLevel, 48000));
    QVERIFY(!effect->isSettled());
    // The slow stage takes seconds
    process(*effect, std::vector<double>(static_cast<size_t>(rate) * 2 * 30, 0.0));
    QVERIFY(effect->isSettled());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Compressor2ATest)
