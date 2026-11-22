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

#include "compressor_76_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/dsp/fet_compressor_core.hpp"
#include "../../compressor_76.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <set>
#include <vector>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

constexpr double Rate { 48000.0 };
constexpr double DeviceLevel { 0.05 };

void setParameter(Compressor76 & effect, const QString & key, float value)
{
    if (const auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

std::unique_ptr<Compressor76> makeCompressor(double inputDb = 0.0, Compressor76::Ratio ratio = Compressor76::Ratio::FourToOne)
{
    auto effect = std::make_unique<Compressor76>();
    effect->setSampleRate(Rate);
    setParameter(*effect, C::xmlKeyInputGain(), Compressor76::inputGainPosition(inputDb));
    setParameter(*effect, C::xmlKeyRatio(), static_cast<float>(ratio));
    return effect;
}

std::vector<double> sine(double hz, double amplitude, size_t frames, double sampleRate = Rate)
{
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        stereo[i * 2] = stereo[i * 2 + 1] = amplitude * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(i) / sampleRate);
    }
    return stereo;
}

std::vector<double> process(Compressor76 & effect, std::vector<double> stereo, uint32_t sampleRate = static_cast<uint32_t>(Rate), uint8_t oversampleFactor = 1)
{
    const auto frames = static_cast<uint32_t>(stereo.size() / 2);
    AudioContext context { std::span(stereo.data(), stereo.size()), frames, sampleRate };
    context.oversampleFactor = oversampleFactor;
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

FetCompressorCore makeCore(FetCompressorCore::Ratio ratio, double attackSeconds = 0.0002, double releaseSeconds = 0.2, double sampleRate = Rate)
{
    FetCompressorCore core;
    core.setSampleRate(sampleRate);
    core.setRatio(ratio);
    core.setAttackSeconds(attackSeconds);
    core.setReleaseSeconds(releaseSeconds);
    return core;
}

double burstStep(FetCompressorCore & core, double amplitude, double seconds, double sampleRate = Rate)
{
    const auto frames = static_cast<size_t>(seconds * sampleRate);
    for (size_t i = 0; i < frames; i++) {
        core.processGainDb(amplitude, amplitude);
    }
    return -core.reductionDb();
}

double burstSine(FetCompressorCore & core, double amplitude, double seconds, double hz = 1000.0, double sampleRate = Rate)
{
    const auto frames = static_cast<size_t>(seconds * sampleRate);
    for (size_t i = 0; i < frames; i++) {
        const double x = amplitude * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(i) / sampleRate);
        core.processGainDb(x, x);
    }
    return -core.reductionDb();
}

double releaseTime(FetCompressorCore & core, double fraction)
{
    const double start = -core.reductionDb();
    for (size_t i = 0; i < static_cast<size_t>(10.0 * Rate); i++) {
        if (-core.processGainDb(0.0, 0.0) <= start * fraction) {
            return static_cast<double>(i) / Rate;
        }
    }
    return 10.0;
}

std::pair<double, double> harmonicDb(const std::vector<double> & stereo, size_t fromFrame, double fundamentalHz, double sampleRate = Rate)
{
    double re1 = 0.0, im1 = 0.0;
    double re2 = 0.0, im2 = 0.0;
    double re3 = 0.0, im3 = 0.0;
    for (size_t i = fromFrame; i < stereo.size() / 2; i++) {
        const double phase = 2.0 * std::numbers::pi * fundamentalHz * static_cast<double>(i) / sampleRate;
        const double x = stereo[i * 2];
        re1 += x * std::cos(phase);
        im1 += x * std::sin(phase);
        re2 += x * std::cos(2.0 * phase);
        im2 += x * std::sin(2.0 * phase);
        re3 += x * std::cos(3.0 * phase);
        im3 += x * std::sin(3.0 * phase);
    }
    const double fund = std::hypot(re1, im1);
    return { db(std::hypot(re2, im2) / fund), db(std::hypot(re3, im3) / fund) };
}

} // namespace

void Compressor76Test::test_input_minimum_shouldPassWithoutReduction()
{
    for (const auto ratio : { Compressor76::Ratio::FourToOne, Compressor76::Ratio::TwentyToOne, Compressor76::Ratio::All }) {
        auto effect = makeCompressor(-30.0, ratio);
        const auto input = sine(1000.0, 1.0, 48000);
        process(*effect, input);
        QCOMPARE(effect->reductionDb(), 0.0f);
    }
}

void Compressor76Test::test_input_raised_shouldCompressDeviceLevel()
{
    auto effect = makeCompressor(12.0, Compressor76::Ratio::FourToOne);
    const auto input = sine(1000.0, DeviceLevel, 48000);
    const auto output = process(*effect, input);
    const double netChange = db(rms(output, 24000) / rms(input, 24000));
    // +12 dB input drives -26 dBFS to -14 dBFS (+12 dB over threshold at 4:1 => ~9 dB GR -> net +3 dB)
    QVERIFY(effect->reductionDb() < -7.0f);
    QVERIFY2(netChange < 6.0 && netChange > 0.0, qPrintable(QString::number(netChange)));
}

void Compressor76Test::test_staticCurve_ratios_shouldMatchSlopesAndSmoothKnee()
{
    const std::array<std::pair<FetCompressorCore::Ratio, double>, 4> expectedSlopes { {
      { FetCompressorCore::Ratio::FourToOne, 1.0 - 1.0 / 4.0 },
      { FetCompressorCore::Ratio::EightToOne, 1.0 - 1.0 / 8.0 },
      { FetCompressorCore::Ratio::TwelveToOne, 1.0 - 1.0 / 12.0 },
      { FetCompressorCore::Ratio::TwentyToOne, 1.0 - 1.0 / 20.0 },
    } };

    for (auto && [ratio, expected] : expectedSlopes) {
        const auto core = makeCore(ratio);
        const double slope = (core.targetReductionDb(60.0) - core.targetReductionDb(40.0)) / 20.0;
        QVERIFY2(std::abs(slope - expected) < 0.005, qPrintable(QString::number(slope)));
    }

    // All-Buttons-In engages earlier / reduces strictly more than 20:1 across working levels
    const auto twenty = makeCore(FetCompressorCore::Ratio::TwentyToOne);
    const auto all = makeCore(FetCompressorCore::Ratio::All);
    for (const double levelDb : { -22.0, -15.0, -5.0, 10.0 }) {
        QVERIFY2(all.targetReductionDb(levelDb) > twenty.targetReductionDb(levelDb), qPrintable(QString::number(levelDb)));
    }

    // Smooth knee: slope increases monotonically without corners
    const auto four = makeCore(FetCompressorCore::Ratio::FourToOne);
    double previousSlope = 0.0;
    for (double level = -45.0; level < 10.0; level += 0.1) {
        const double slope = (four.targetReductionDb(level + 0.1) - four.targetReductionDb(level)) / 0.1;
        QVERIFY2(slope >= previousSlope - 1.0e-9, qPrintable(QString::number(level)));
        QVERIFY2(slope - previousSlope < 0.05, qPrintable(QString::number(level)));
        previousSlope = slope;
    }
}

void Compressor76Test::test_stability_extremeFeedback_shouldStayFiniteAndConverge()
{
    for (const double sr : { 44100.0, 48000.0, 96000.0 }) {
        for (const auto ratio : { FetCompressorCore::Ratio::TwentyToOne, FetCompressorCore::Ratio::All }) {
            auto core = makeCore(ratio, 0.00002, 0.05, sr);
            const double expected = core.targetReductionDb(0.0);
            const auto frames = static_cast<size_t>(0.5 * sr);
            for (size_t i = 0; i < frames; i++) {
                const double gr = core.processGainDb(1.0, 1.0);
                QVERIFY(std::isfinite(gr));
                QVERIFY(gr <= 0.0);
            }
            QVERIFY2(std::abs(-core.reductionDb() - expected) < 0.05, qPrintable(QString { "got %1 expected %2" }.arg(-core.reductionDb()).arg(expected)));
        }
    }

    for (const uint8_t factor : { 1, 2, 4 }) {
        auto effect = makeCompressor(24.0, Compressor76::Ratio::All);
        setParameter(*effect, C::xmlKeyAttack(), 0.0f);
        setParameter(*effect, C::xmlKeyRelease(), 0.0f);
        const auto output = process(*effect, sine(1000.0, 1.0, 24000), 48000, factor);
        for (const double s : output) {
            QVERIFY(std::isfinite(s));
        }
    }
}

void Compressor76Test::test_attack_fastVsSlow_shouldClampAtExpectedSpeed()
{
    // Faster attack pulls down further in the first 0.25 ms than slowest attack (0.8 ms)
    auto fast = makeCore(FetCompressorCore::Ratio::FourToOne, 0.00002, 0.2);
    auto slow = makeCore(FetCompressorCore::Ratio::FourToOne, 0.0008, 0.2);
    const double fastEarly = burstStep(fast, 0.5, 0.00025);
    const double slowEarly = burstStep(slow, 0.5, 0.00025);
    QVERIFY2(fastEarly > slowEarly * 2.0, qPrintable(QString { "fast=%1 slow=%2" }.arg(fastEarly).arg(slowEarly)));

    // Feedback tightening: at identical attack knob, 20:1 reaches a larger share of its target
    // within 0.2 ms than 4:1 does
    auto four = makeCore(FetCompressorCore::Ratio::FourToOne, 0.0008, 0.2);
    auto twenty = makeCore(FetCompressorCore::Ratio::TwentyToOne, 0.0008, 0.2);
    const double fourShare = burstStep(four, 0.5, 0.0002) / four.targetReductionDb(db(0.5));
    const double twentyShare = burstStep(twenty, 0.5, 0.0002) / twenty.targetReductionDb(db(0.5));
    QVERIFY2(twentyShare > fourShare * 1.5, qPrintable(QString { "four=%1 twenty=%2" }.arg(fourShare).arg(twentyShare)));
}

void Compressor76Test::test_attack_allMode_shouldPreserveTransientSnap()
{
    auto twenty = makeCore(FetCompressorCore::Ratio::TwentyToOne, 0.00002, 0.2);
    auto all = makeCore(FetCompressorCore::Ratio::All, 0.00002, 0.2);
    const double twentyFraction = burstStep(twenty, 0.5, 0.0002) / twenty.targetReductionDb(db(0.5));
    const double allFraction = burstStep(all, 0.5, 0.0002) / all.targetReductionDb(db(0.5));
    QVERIFY2(twentyFraction > 0.85, qPrintable(QString::number(twentyFraction)));
    QVERIFY2(allFraction < 0.45, qPrintable(QString::number(allFraction)));
    const double allSettledFraction = burstStep(all, 0.5, 0.004) / all.targetReductionDb(db(0.5));
    QVERIFY2(allSettledFraction > 0.95, qPrintable(QString::number(allSettledFraction)));
}

void Compressor76Test::test_release_fastVsSlow_andProgramDependence()
{
    auto fast = makeCore(FetCompressorCore::Ratio::FourToOne, 0.0002, 0.05);
    auto slow = makeCore(FetCompressorCore::Ratio::FourToOne, 0.0002, 1.1);
    burstSine(fast, 0.5, 0.5);
    burstSine(slow, 0.5, 0.5);
    const double fastTime = releaseTime(fast, 0.2);
    const double slowTime = releaseTime(slow, 0.2);
    QVERIFY2(fastTime < 0.15, qPrintable(QString::number(fastTime)));
    QVERIFY2(slowTime > 1.0 && slowTime < 5.0, qPrintable(QString::number(slowTime)));

    // Two-stage program dependence: a long sustained burst builds up the slow stage and recovers
    // slower than a brief transient at the exact same Release setting
    auto shortCore = makeCore(FetCompressorCore::Ratio::FourToOne, 0.0002, 0.2);
    auto longCore = makeCore(FetCompressorCore::Ratio::FourToOne, 0.0002, 0.2);
    burstSine(shortCore, 0.5, 0.03);
    burstSine(longCore, 0.5, 1.5);
    const double shortRecovery = releaseTime(shortCore, 0.15);
    const double longRecovery = releaseTime(longCore, 0.15);
    QVERIFY2(longRecovery > shortRecovery * 1.3, qPrintable(QString { "short=%1 long=%2" }.arg(shortRecovery).arg(longRecovery)));
}

void Compressor76Test::test_lowCut_shouldKeepSubBassOutOfDetector()
{
    auto effect = makeCompressor(10.0, Compressor76::Ratio::EightToOne);
    setParameter(*effect, C::xmlKeyHpfCutoff(), Compressor76::lowCutPosition(250.0));

    const auto subInput = sine(40.0, 0.06, 48000);
    const auto subOutput = process(*effect, subInput);
    const float subGr = effect->reductionDb();

    effect->reset();
    const auto midInput = sine(1000.0, 0.06, 48000);
    process(*effect, midInput);
    const float midGr = effect->reductionDb();

    QVERIFY2(std::abs(subGr) < 0.5f, qPrintable(QString::number(subGr)));
    QVERIFY2(midGr < -6.0f, qPrintable(QString::number(midGr)));

    // Verify the audio path itself is not high-passed (+10 dB input with ~0 dB GR => ~+10 dB output)
    const double subGainDb = db(rms(subOutput, 24000) / rms(subInput, 24000));
    QVERIFY2(std::abs(subGainDb - 10.0) < 0.6, qPrintable(QString::number(subGainDb)));
}

void Compressor76Test::test_output_shouldMoveOutputLevel()
{
    const auto level = [](double outputDb) {
        auto effect = makeCompressor(-30.0, Compressor76::Ratio::FourToOne);
        setParameter(*effect, C::xmlKeyGain(), Compressor76::outputGainPosition(outputDb));
        return rms(process(*effect, sine(1000.0, DeviceLevel, 48000)), 24000);
    };
    const double change = db(level(10.0) / level(0.0));
    QVERIFY2(std::abs(change - 10.0) < 0.15, qPrintable(QString::number(change)));
}

void Compressor76Test::test_character_blueStripeVsBlackface_shouldAddMoreHarmonicsAndPresence()
{
    // Hot compressed signal: check harmonics, All-mode grit, Blue Stripe > Blackface, and DC blocking
    auto blackface = makeCompressor(12.0, Compressor76::Ratio::FourToOne);
    setParameter(*blackface, C::xmlKeyMode(), static_cast<float>(Compressor76::Character::Blackface));
    const auto bfOut = process(*blackface, sine(1000.0, 0.25, 48000), 48000, 4);

    auto blueStripe = makeCompressor(12.0, Compressor76::Ratio::FourToOne);
    setParameter(*blueStripe, C::xmlKeyMode(), static_cast<float>(Compressor76::Character::BlueStripe));
    const auto bsOut = process(*blueStripe, sine(1000.0, 0.25, 48000), 48000, 4);

    const auto [bfH2, bfH3] = harmonicDb(bfOut, 24000, 1000.0);
    const auto [bsH2, bsH3] = harmonicDb(bsOut, 24000, 1000.0);
    QVERIFY2(bfH2 > -55.0, qPrintable(QString::number(bfH2)));
    QVERIFY2(bsH2 > bfH2 + 2.0, qPrintable(QString { "bf=%1 bs=%2" }.arg(bfH2).arg(bsH2)));
    QVERIFY2(bsH3 > bfH3 + 2.0, qPrintable(QString { "bf=%1 bs=%2" }.arg(bfH3).arg(bsH3)));

    // DC blocker removes asymmetric rectifier/shaper offset
    double sum = 0.0;
    for (size_t i = 24000; i < bsOut.size() / 2; i++) {
        sum += bsOut[i * 2];
    }
    const double dcMean = sum / 24000.0;
    QVERIFY2(std::abs(dcMean) < 1.0e-4, qPrintable(QString::number(dcMean)));

    // All-Buttons-In adds substantially more FET harmonic grit than 20:1
    auto twenty = makeCompressor(12.0, Compressor76::Ratio::TwentyToOne);
    auto all = makeCompressor(12.0, Compressor76::Ratio::All);
    const auto [h2Twenty, h3Twenty] = harmonicDb(process(*twenty, sine(1000.0, 0.25, 48000), 48000, 4), 24000, 1000.0);
    const auto [h2All, h3All] = harmonicDb(process(*all, sine(1000.0, 0.25, 48000), 48000, 4), 24000, 1000.0);
    QVERIFY2(h2All > h2Twenty + 3.0 && h3All > h3Twenty + 3.0, qPrintable(QString { "20=(%1,%2) All=(%3,%4)" }.arg(h2Twenty).arg(h3Twenty).arg(h2All).arg(h3All)));

    // Blue Stripe presence bell around 2 kHz lifts 2 kHz relative to 100 Hz even below threshold
    const auto ratioAt = [](Compressor76::Character character, double hz) {
        auto effect = makeCompressor(-30.0, Compressor76::Ratio::FourToOne);
        setParameter(*effect, C::xmlKeyMode(), static_cast<float>(character));
        const auto in = sine(hz, DeviceLevel, 48000);
        const auto out = process(*effect, in, 48000, 2);
        return db(rms(out, 24000) / rms(in, 24000));
    };
    const double bfTilt = ratioAt(Compressor76::Character::Blackface, 2000.0) - ratioAt(Compressor76::Character::Blackface, 100.0);
    const double bsTilt = ratioAt(Compressor76::Character::BlueStripe, 2000.0) - ratioAt(Compressor76::Character::BlueStripe, 100.0);
    QVERIFY2(std::abs(bfTilt) < 0.1, qPrintable(QString::number(bfTilt)));
    QVERIFY2(bsTilt > 0.9 && bsTilt < 1.5, qPrintable(QString::number(bsTilt)));
}

void Compressor76Test::test_oversampling_shouldMatchLevelAndReduceAliasing()
{
    const auto input = sine(1000.0, DeviceLevel, 48000);
    std::array<double, 3> levels {};
    const std::array<uint8_t, 3> factors { 1, 2, 4 };
    for (size_t i = 0; i < factors.size(); i++) {
        auto effect = makeCompressor(12.0, Compressor76::Ratio::EightToOne);
        const auto output = process(*effect, input, 48000, factors[i]);
        levels[i] = rms(output, 24000);
    }
    QVERIFY2(std::abs(db(levels[1] / levels[0])) < 0.3, qPrintable(QString::number(db(levels[1] / levels[0]))));
    QVERIFY2(std::abs(db(levels[2] / levels[1])) < 0.2, qPrintable(QString::number(db(levels[2] / levels[1]))));

    // Hot 15 kHz in All + BlueStripe with +12 dB output: 3rd harmonic (45 kHz) folds back to 3 kHz at 1x,
    // while 4x oversampling places 45 kHz below Nyquist and removes it on decimation.
    const auto measureAlias3k = [](uint8_t factor) {
        auto effect = makeCompressor(12.0, Compressor76::Ratio::All);
        setParameter(*effect, C::xmlKeyGain(), Compressor76::outputGainPosition(12.0));
        setParameter(*effect, C::xmlKeyMode(), static_cast<float>(Compressor76::Character::BlueStripe));
        const auto out = process(*effect, sine(15000.0, 0.3, 48000), 48000, factor);
        double reFund = 0.0, imFund = 0.0, reAlias = 0.0, imAlias = 0.0;
        for (size_t i = 24000; i < 48000; i++) {
            const double pFund = 2.0 * std::numbers::pi * 15000.0 * static_cast<double>(i) / 48000.0;
            const double pAlias = 2.0 * std::numbers::pi * 3000.0 * static_cast<double>(i) / 48000.0;
            reFund += out[i * 2] * std::cos(pFund);
            imFund += out[i * 2] * std::sin(pFund);
            reAlias += out[i * 2] * std::cos(pAlias);
            imAlias += out[i * 2] * std::sin(pAlias);
        }
        return db(std::hypot(reAlias, imAlias) / std::hypot(reFund, imFund));
    };
    const double alias1x = measureAlias3k(1);
    const double alias4x = measureAlias3k(4);
    QVERIFY2(alias4x < alias1x - 20.0, qPrintable(QString { "1x=%1 4x=%2" }.arg(alias1x).arg(alias4x)));
}

void Compressor76Test::test_mix_zeroAndParallel_andSendMode()
{
    auto effect = makeCompressor(18.0, Compressor76::Ratio::TwentyToOne);
    setParameter(*effect, C::xmlKeyMix(), 0.0f);
    const auto input = sine(1000.0, DeviceLevel, 4800);
    QCOMPARE(process(*effect, input, 48000, 1), input);

    // Parallel Mix=0.5 under 4x oversampling must stay phase-aligned at high frequencies (no comb notch)
    auto wetOnly = makeCompressor(-30.0, Compressor76::Ratio::FourToOne);
    auto halfMix = makeCompressor(-30.0, Compressor76::Ratio::FourToOne);
    setParameter(*halfMix, C::xmlKeyMix(), 0.5f);
    const auto highTone = sine(8000.0, DeviceLevel, 24000);
    const double wetRms = rms(process(*wetOnly, highTone, 48000, 4), 12000);
    const double halfRms = rms(process(*halfMix, highTone, 48000, 4), 12000);
    // At -30 dB Input and 0 dB Output, wet is -30 dB below dry, so 50% dry + 50% quiet wet is ~ -6 dB below dry
    const double diffDb = db(halfRms / rms(highTone, 12000));
    QVERIFY(wetRms < rms(highTone, 12000) * 0.1);
    QVERIFY2(std::abs(diffDb - (-6.02)) < 0.3, qPrintable(QString::number(diffDb)));

    // Send mode passes dry + wet * mix without double-counting or comb residual
    auto sendEffect = makeCompressor(-30.0, Compressor76::Ratio::FourToOne);
    setParameter(*sendEffect, C::xmlKeyMix(), 0.5f);
    sendEffect->setSendMode(true);
    const auto sendOut = process(*sendEffect, highTone, 48000, 4);
    const double sendRms = rms(sendOut, 12000);
    QVERIFY2(std::abs(db(sendRms / rms(highTone, 12000))) < 0.3, qPrintable(QString::number(db(sendRms / rms(highTone, 12000)))));
}

void Compressor76Test::test_isSettled_andReset_shouldSettleAndRenderIdentically()
{
    auto effect = makeCompressor(12.0, Compressor76::Ratio::EightToOne);
    QVERIFY(effect->isSettled());
    const auto input = sine(1000.0, DeviceLevel, 24000);
    const auto firstRender = process(*effect, input, 48000, 2);
    QVERIFY(!effect->isSettled());

    process(*effect, std::vector<double>(static_cast<size_t>(Rate) * 2 * 5, 0.0), 48000, 2);
    QVERIFY(effect->isSettled());

    effect->reset();
    QVERIFY(effect->isSettled());
    const auto secondRender = process(*effect, input, 48000, 2);
    QCOMPARE(secondRender, firstRender);
}

void Compressor76Test::test_factoryPresets_shouldHaveUniqueNamesAndRenderFinite()
{
    Compressor76 effect;
    effect.setSampleRate(Rate);
    const auto & presets = effect.factoryPresets();
    QVERIFY(!presets.empty());
    QCOMPARE(presets.front().name, std::string { "Init" });

    std::set<std::string> names;
    const auto input = sine(1000.0, DeviceLevel, 4800);
    for (size_t i = 0; i < presets.size(); i++) {
        QVERIFY2(names.insert(presets[i].name).second, presets[i].name.c_str());
        QVERIFY(effect.applyFactoryPreset(i));
        effect.reset();
        const auto out = process(effect, input, 48000, 2);
        for (const double s : out) {
            QVERIFY(std::isfinite(s));
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Compressor76Test)
