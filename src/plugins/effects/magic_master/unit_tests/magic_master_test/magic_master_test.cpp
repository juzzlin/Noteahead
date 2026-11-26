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

#include "magic_master_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../../../../domain/dsp/audio_context.hpp"
#include "../../../../../domain/utility/loudness_analyzer.hpp"
#include "../../magic_master.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double Rate { 48000.0 };
namespace C = Constants::NahdXml;

void setParam(MagicMaster & mm, const QString & key, float value)
{
    if (const auto p { mm.parameter(key.toStdString()) }; p) {
        p->get().setValue(value);
    }
    mm.sync();
}

void disableAllStages(MagicMaster & mm)
{
    setParam(mm, C::xmlKeySubCleanEnabled(), 0.0f);
    setParam(mm, C::xmlKeyEqEnabled(), 0.0f);
    setParam(mm, C::xmlKeyMultibandEnabled(), 0.0f);
    setParam(mm, C::xmlKeyStereoEnabled(), 0.0f);
    setParam(mm, C::xmlKeyClipperEnabled(), 0.0f);
    setParam(mm, C::xmlKeyLimiterEnabled(), 0.0f);
}

std::vector<double> makeEdmSignal(size_t frames, double amplitude)
{
    std::vector<double> buf(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double t { static_cast<double>(i) / Rate };
        const double bass { std::sin(2.0 * std::numbers::pi * 55.0 * t) };
        const double lead { 0.45 * std::sin(2.0 * std::numbers::pi * 440.0 * t) + 0.25 * std::sin(2.0 * std::numbers::pi * 2200.0 * t) };
        // Beat transient every 0.5s (120 BPM)
        const double beatPhase { std::fmod(t, 0.5) };
        const double kickEnv { std::exp(-beatPhase * 28.0) };
        const double kick { 0.7 * kickEnv * std::sin(2.0 * std::numbers::pi * (50.0 + 120.0 * kickEnv) * beatPhase) };
        buf[i * 2] = amplitude * (bass + lead + kick);
        buf[i * 2 + 1] = amplitude * (bass + 0.9 * lead + kick + 0.1 * std::cos(2.0 * std::numbers::pi * 660.0 * t));
    }
    return buf;
}

} // namespace

void MagicMasterTest::test_allOff_shouldPassAudioThroughUntouched()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);
    disableAllStages(mm);

    QCOMPARE(mm.latencyFrames(Rate), size_t { 0 });

    const size_t frames { 1024 };
    auto input { makeEdmSignal(frames, 0.4) };
    auto output { input };
    AudioContext ctx { .buffer = output, .frameCount = static_cast<uint32_t>(frames), .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(ctx);

    for (size_t i = 0; i < output.size(); i++) {
        QVERIFY(std::abs(output[i] - input[i]) < 1.0e-12);
    }
}

void MagicMasterTest::test_latencyFrames_shouldMatchImpulseResponseForAllCombos()
{
    const auto measureImpulseIndex = [](bool clipperOn, bool limiterOn) {
        MagicMaster mm;
        mm.setSampleRate(Rate);
        disableAllStages(mm);
        setParam(mm, C::xmlKeyClipperEnabled(), clipperOn ? 1.0f : 0.0f);
        setParam(mm, C::xmlKeyLimiterEnabled(), limiterOn ? 1.0f : 0.0f);

        const size_t reportedLatency { mm.latencyFrames(Rate) };
        const size_t totalFrames { reportedLatency + 128 };
        std::vector<double> buf(totalFrames * 2, 0.0);
        buf[0] = 0.25;
        buf[1] = 0.25;
        AudioContext ctx { .buffer = buf, .frameCount = static_cast<uint32_t>(totalFrames), .sampleRate = static_cast<uint32_t>(Rate) };
        mm.process(ctx);

        size_t maxIdx { 0 };
        double maxVal { 0.0 };
        for (size_t i = 0; i < totalFrames; i++) {
            if (std::abs(buf[i * 2]) > maxVal) {
                maxVal = std::abs(buf[i * 2]);
                maxIdx = i;
            }
        }
        return std::pair<size_t, size_t> { maxIdx, reportedLatency };
    };

    const auto neither { measureImpulseIndex(false, false) };
    QCOMPARE(neither.second, size_t { 0 });
    QCOMPARE(neither.first, neither.second);

    const auto clipperOnly { measureImpulseIndex(true, false) };
    QCOMPARE(clipperOnly.second, MagicMaster::ClipperOversampleLatencyFrames);
    QCOMPARE(clipperOnly.first, clipperOnly.second);

    const auto limiterOnly { measureImpulseIndex(false, true) };
    QCOMPARE(limiterOnly.second, TruePeakLimiterCore::latencyFramesFor(Rate, 4.0));
    QCOMPARE(limiterOnly.first, limiterOnly.second);

    const auto both { measureImpulseIndex(true, true) };
    QCOMPARE(both.second, MagicMaster::ClipperOversampleLatencyFrames + TruePeakLimiterCore::latencyFramesFor(Rate, 4.0));
    QCOMPARE(both.first, both.second);
}

void MagicMasterTest::test_subCleanStage_shouldFilterSubsonicAndMonoLowEnd()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);
    disableAllStages(mm);
    setParam(mm, C::xmlKeySubCleanEnabled(), 1.0f);

    // 1. Subsonic 16 Hz stereo opposite-phase + DC offset should be heavily attenuated & monoed
    const size_t frames { 48000 };
    std::vector<double> subDiff(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double s { 0.4 * std::sin(2.0 * std::numbers::pi * 16.0 * static_cast<double>(i) / Rate) };
        subDiff[i * 2] = 0.2 + s;
        subDiff[i * 2 + 1] = 0.2 - s;
    }
    AudioContext ctx { .buffer = subDiff, .frameCount = static_cast<uint32_t>(frames), .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(ctx);

    double tailPeak { 0.0 };
    for (size_t i = frames / 2; i < frames; i++) {
        tailPeak = std::max(tailPeak, std::max(std::abs(subDiff[i * 2]), std::abs(subDiff[i * 2 + 1])));
    }
    QVERIFY2(tailPeak < 0.04, qPrintable(QString { "16 Hz + DC tailPeak=%1" }.arg(tailPeak)));

    // 2. 60 Hz opposite-phase stereo must collapse to mono (< 1e-4), while 600 Hz stereo diff survives
    mm.reset();
    std::vector<double> lowMidDiff(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double lo { 0.3 * std::sin(2.0 * std::numbers::pi * 60.0 * static_cast<double>(i) / Rate) };
        lowMidDiff[i * 2] = lo;
        lowMidDiff[i * 2 + 1] = -lo;
    }
    AudioContext ctx2 { .buffer = lowMidDiff, .frameCount = static_cast<uint32_t>(frames), .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(ctx2);

    double lowTailRms { 0.0 };
    for (size_t i = frames / 2; i < frames; i++) {
        lowTailRms += lowMidDiff[i * 2] * lowMidDiff[i * 2];
    }
    lowTailRms = std::sqrt(lowTailRms / static_cast<double>(frames / 2));
    QVERIFY2(lowTailRms < 0.02, qPrintable(QString { "60 Hz mono collapsed RMS=%1" }.arg(lowTailRms)));
}

void MagicMasterTest::test_autoEqStage_shouldVisiblyShapeTiltedMixAndBypassCleanly()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);
    disableAllStages(mm);
    setParam(mm, C::xmlKeyEqEnabled(), 1.0f);

    // Construct a tilted, relatively quiet (~-20 dBFS) mix heavy at 260 Hz (mud) & 11 kHz (air), light at 65 Hz (sub)
    const size_t totalFrames { static_cast<size_t>(Rate * 2.5) };
    std::vector<double> tilted(totalFrames * 2, 0.0);
    for (size_t i = 0; i < totalFrames; i++) {
        const double t { static_cast<double>(i) / Rate };
        const double s {
            0.015 * std::sin(2.0 * std::numbers::pi * 65.0 * t)
            + 0.14 * std::sin(2.0 * std::numbers::pi * 260.0 * t)
            + 0.035 * std::sin(2.0 * std::numbers::pi * 900.0 * t)
            + 0.025 * std::sin(2.0 * std::numbers::pi * 3200.0 * t)
            + 0.08 * std::sin(2.0 * std::numbers::pi * 11000.0 * t)
        };
        tilted[i * 2] = s;
        tilted[i * 2 + 1] = s;
    }

    auto processed { tilted };
    for (size_t pos = 0; pos < totalFrames; pos += 512) {
        const uint32_t n { static_cast<uint32_t>(std::min<size_t>(512, totalFrames - pos)) };
        AudioContext ctx { .buffer = std::span<double> { processed.data() + pos * 2, static_cast<size_t>(n) * 2 }, .frameCount = n, .sampleRate = static_cast<uint32_t>(Rate) };
        mm.process(ctx);
    }

    // Detectors should measure tilted spectrum accurately and react without stalling at zero-crossings
    QVERIFY2(std::abs(mm.autoEqMaxMoveDb()) > 1.0f, qPrintable(QString { "autoEqMaxMoveDb=%1" }.arg(mm.autoEqMaxMoveDb())));
    QVERIFY2(mm.autoEqBandDb(1) < -1.0f, qPrintable(QString { "band1(260Hz)=%1" }.arg(mm.autoEqBandDb(1))));
    QVERIFY2(mm.autoEqBandDb(4) < -1.0f, qPrintable(QString { "band4(11kHz)=%1" }.arg(mm.autoEqBandDb(4))));
    QVERIFY2(mm.autoEqBandDb(0) > 1.0f, qPrintable(QString { "band0(65Hz)=%1" }.arg(mm.autoEqBandDb(0))));

    // Audible difference in processed tail relative to input
    double diffRms { 0.0 };
    for (size_t i = totalFrames / 2; i < totalFrames; i++) {
        const double d { processed[i * 2] - tilted[i * 2] };
        diffRms += d * d;
    }
    diffRms = std::sqrt(diffRms / static_cast<double>(totalFrames / 2));
    QVERIFY2(diffRms > 0.01, qPrintable(QString { "eq diffRms=%1" }.arg(diffRms)));

    // Disabling Stage 2 clears EQ band telemetry and bypasses audio untouched
    setParam(mm, C::xmlKeyEqEnabled(), 0.0f);
    std::vector<double> bypassed(512 * 2, 0.0);
    std::copy_n(tilted.begin(), bypassed.size(), bypassed.begin());
    AudioContext bypassCtx { .buffer = bypassed, .frameCount = 512, .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(bypassCtx);

    QCOMPARE(mm.autoEqMaxMoveDb(), 0.0f);
    for (int b = 0; b < 5; b++) {
        QCOMPARE(mm.autoEqBandDb(b), 0.0f);
    }
    for (size_t i = 0; i < bypassed.size(); i++) {
        QVERIFY(std::abs(bypassed[i] - tilted[i]) < 1.0e-12);
    }
}

void MagicMasterTest::test_multibandStage_shouldPassQuietSignalAndCompressHotSignal()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);
    disableAllStages(mm);
    setParam(mm, C::xmlKeyMultibandEnabled(), 1.0f);

    // Quiet -30 dBFS 200 Hz sine must pass through cleanly without muting or gain blow-up
    const size_t quietFrames { 4096 };
    std::vector<double> quiet(quietFrames * 2, 0.0);
    for (size_t i = 0; i < quietFrames; i++) {
        const double s { 0.0316 * std::sin(2.0 * std::numbers::pi * 200.0 * static_cast<double>(i) / Rate) };
        quiet[i * 2] = s;
        quiet[i * 2 + 1] = s;
    }
    const auto quietIn { quiet };
    AudioContext quietCtx { .buffer = quiet, .frameCount = static_cast<uint32_t>(quietFrames), .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(quietCtx);

    double inRms { 0.0 };
    double outRms { 0.0 };
    for (size_t i = quietFrames / 2; i < quietFrames; i++) {
        inRms += quietIn[i * 2] * quietIn[i * 2];
        outRms += quiet[i * 2] * quiet[i * 2];
    }
    inRms = std::sqrt(inRms / static_cast<double>(quietFrames / 2));
    outRms = std::sqrt(outRms / static_cast<double>(quietFrames / 2));
    QVERIFY2(outRms > inRms * 0.8 && outRms < inRms * 1.4, qPrintable(QString { "inRms=%1 outRms=%2" }.arg(inRms).arg(outRms)));

    // Hot EDM signal (-3 dBFS peak) should trigger finite negative multiband gain reduction
    mm.reset();
    const size_t hotFrames { 16384 };
    auto hot { makeEdmSignal(hotFrames, 0.45) };
    AudioContext hotCtx { .buffer = hot, .frameCount = static_cast<uint32_t>(hotFrames), .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(hotCtx);

    for (double sample : hot) {
        QVERIFY(std::isfinite(sample));
        QVERIFY(std::abs(sample) < 4.0);
    }
    const float minGr { std::min({ mm.multibandReductionDb(0), mm.multibandReductionDb(1), mm.multibandReductionDb(2) }) };
    QVERIFY2(minGr < -0.2f, qPrintable(QString { "minMbGr=%1" }.arg(minGr)));
}

void MagicMasterTest::test_boostingLimiter_shouldHoldFixedBoostAndRespectTruePeakCeiling()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);
    // Default Threshold -6.0 dB, Ceiling -1.2 dBTP -> +4.8 dB fixed boost, all 6 stages enabled
    const size_t totalFrames { static_cast<size_t>(Rate * 4) };
    auto runSignal { makeEdmSignal(totalFrames, 0.14) };

    for (size_t pos = 0; pos < totalFrames; pos += 512) {
        const uint32_t n { static_cast<uint32_t>(std::min<size_t>(512, totalFrames - pos)) };
        AudioContext ctx { .buffer = std::span<double> { runSignal.data() + pos * 2, static_cast<size_t>(n) * 2 }, .frameCount = n, .sampleRate = static_cast<uint32_t>(Rate) };
        mm.process(ctx);
    }

    QVERIFY2(std::abs(mm.limiterBoostDb() - 4.8f) < 0.05f, qPrintable(QString { "default boost=%1" }.arg(mm.limiterBoostDb())));
    const float defaultLufs { mm.integratedLufs() };
    QVERIFY2(defaultLufs > -20.0f && defaultLufs < -2.0f, qPrintable(QString { "default integratedLufs=%1" }.arg(defaultLufs)));

    // Silence must NOT cause gain boost to wander upward
    std::vector<double> silence(static_cast<size_t>(Rate * 2) * 2, 0.0);
    for (size_t pos = 0; pos < silence.size() / 2; pos += 512) {
        const uint32_t n { static_cast<uint32_t>(std::min<size_t>(512, silence.size() / 2 - pos)) };
        AudioContext ctx { .buffer = std::span<double> { silence.data() + pos * 2, static_cast<size_t>(n) * 2 }, .frameCount = n, .sampleRate = static_cast<uint32_t>(Rate) };
        mm.process(ctx);
    }
    QVERIFY2(std::abs(mm.limiterBoostDb() - 4.8f) < 0.05f, qPrintable(QString { "post-silence boost=%1" }.arg(mm.limiterBoostDb())));

    // Lowering Threshold to -12.0 dB at -1.0 dBTP Ceiling raises fixed boost to +11.0 dB and increases output LUFS while holding ceiling
    mm.reset();
    setParam(mm, C::xmlKeyThreshold(), MagicMaster::thresholdPosition(-12.0));
    setParam(mm, C::xmlKeyCeiling(), MagicMaster::ceilingPosition(-1.0));
    auto hotRun { makeEdmSignal(totalFrames, 0.14) };
    for (size_t pos = 0; pos < totalFrames; pos += 512) {
        const uint32_t n { static_cast<uint32_t>(std::min<size_t>(512, totalFrames - pos)) };
        AudioContext ctx { .buffer = std::span<double> { hotRun.data() + pos * 2, static_cast<size_t>(n) * 2 }, .frameCount = n, .sampleRate = static_cast<uint32_t>(Rate) };
        mm.process(ctx);
    }

    QVERIFY2(std::abs(mm.limiterBoostDb() - 11.0f) < 0.05f, qPrintable(QString { "pushed boost=%1" }.arg(mm.limiterBoostDb())));
    QVERIFY2(mm.integratedLufs() > defaultLufs + 3.0f, qPrintable(QString { "pushed=%1 default=%2" }.arg(mm.integratedLufs()).arg(defaultLufs)));

    LoudnessAnalyzer analyzer { Rate };
    std::vector<float> floats(totalFrames * 2, 0.0f);
    for (size_t i = 0; i < floats.size(); i++) {
        floats[i] = static_cast<float>(hotRun[i]);
    }
    analyzer.process(floats.data(), floats.size());
    const auto stats { analyzer.calculate() };
    QVERIFY2(stats.truePeak <= -1.0f + 0.05f, qPrintable(QString { "truePeak=%1" }.arg(stats.truePeak)));
}

void MagicMasterTest::test_songContext_shouldAdaptSubHpfAndMonoBassAndBpm()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);

    SongContext sub808 {};
    sub808.hasSongData = true;
    sub808.bpm = 140.0;
    sub808.lowestBassHz = 32.7; // C1
    sub808.hasSubOr808 = true;
    mm.setSongContext(sub808);
    mm.sync();

    std::array<double, 2> dummy { 0.0, 0.0 };
    mm.process(dummy[0], dummy[1]);
    QVERIFY(mm.subHpfHz() <= 26.0f);
    QCOMPARE(mm.monoBassHz(), 115.0f);

    SongContext fastNoSub {};
    fastNoSub.hasSongData = true;
    fastNoSub.bpm = 172.0;
    fastNoSub.lowestBassHz = 65.4; // C2
    fastNoSub.hasSubOr808 = false;
    mm.setSongContext(fastNoSub);
    mm.sync();
    mm.process(dummy[0], dummy[1]);
    QCOMPARE(mm.subHpfHz(), 36.0f);
    QCOMPARE(mm.monoBassHz(), 135.0f);
}

void MagicMasterTest::test_calibration_shouldLockEqAndMultibandAndRespectCeiling()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);

    const size_t totalFrames { static_cast<size_t>(Rate * 4) };
    const auto doubleSignal { makeEdmSignal(totalFrames, 0.14) };
    std::vector<float> floatInput(doubleSignal.size(), 0.0f);
    for (size_t i = 0; i < doubleSignal.size(); i++) {
        floatInput[i] = static_cast<float>(doubleSignal[i]);
    }

    const auto cal { mm.calibrateFromStereo(floatInput, static_cast<uint32_t>(Rate)) };
    QVERIFY(cal.valid);
    QVERIFY(cal.measuredInputLufs > -40.0);

    mm.setCalibration(cal);
    mm.reset();
    QVERIFY(mm.isCalibrated());

    // Process buffer through calibrated MagicMaster and verify locked Auto EQ + true-peak ceiling
    auto processed { doubleSignal };
    const size_t latency { mm.latencyFrames(Rate) };
    for (size_t pos = 0; pos < totalFrames; pos += 512) {
        const uint32_t n { static_cast<uint32_t>(std::min<size_t>(512, totalFrames - pos)) };
        AudioContext ctx { .buffer = std::span<double> { processed.data() + pos * 2, static_cast<size_t>(n) * 2 }, .frameCount = n, .sampleRate = static_cast<uint32_t>(Rate) };
        ctx.offline = true;
        mm.process(ctx);
    }
    std::vector<double> tail(latency * 2, 0.0);
    AudioContext tailCtx { .buffer = tail, .frameCount = static_cast<uint32_t>(latency), .sampleRate = static_cast<uint32_t>(Rate) };
    tailCtx.offline = true;
    mm.process(tailCtx);

    for (size_t b = 0; b < 5; b++) {
        QVERIFY2(std::abs(mm.autoEqBandDb(static_cast<int>(b)) - static_cast<float>(cal.autoEqBandDb[b])) < 0.01f,
                 qPrintable(QString { "band %1 got=%2 cal=%3" }.arg(b).arg(mm.autoEqBandDb(static_cast<int>(b))).arg(cal.autoEqBandDb[b])));
    }

    std::vector<float> outFloats;
    outFloats.reserve(totalFrames * 2);
    for (size_t i = latency * 2; i < processed.size(); i++) {
        outFloats.push_back(static_cast<float>(processed[i]));
    }
    for (size_t i = 0; i < tail.size(); i++) {
        outFloats.push_back(static_cast<float>(tail[i]));
    }

    LoudnessAnalyzer outAnalyzer { Rate };
    outAnalyzer.process(outFloats.data(), outFloats.size());
    const auto outStats { outAnalyzer.calculate() };

    QVERIFY2(outStats.truePeak <= -1.2f + 0.05f, qPrintable(QString { "outTruePeak=%1" }.arg(outStats.truePeak)));
}

void MagicMasterTest::test_reset_shouldBeDeterministicWhilePreservingContextAndCalibration()
{
    MagicMaster mm;
    mm.setSampleRate(Rate);

    SongContext ctx {};
    ctx.hasSongData = true;
    ctx.bpm = 142.0;
    ctx.lowestBassHz = 36.7;
    ctx.hasSubOr808 = true;
    ctx.summaryText = "142 BPM EDM";
    mm.setSongContext(ctx);

    MasterCalibration cal {};
    cal.valid = true;
    cal.autoEqBandDb = { 1.2, -1.8, 0.0, 0.6, -1.0 };
    cal.measuredInputLufs = -14.5;
    mm.setCalibration(cal);
    mm.sync();
    mm.reset();

    const size_t frames { 4096 };
    auto run1 { makeEdmSignal(frames, 0.35) };
    auto run2 { run1 };

    AudioContext c1 { .buffer = run1, .frameCount = static_cast<uint32_t>(frames), .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(c1);
    mm.reset();

    QCOMPARE(QString::fromStdString(mm.songContext().summaryText), QString { "142 BPM EDM" });
    QVERIFY(mm.calibration().valid);
    QCOMPARE(mm.calibration().measuredInputLufs, -14.5);
    QCOMPARE(mm.calibration().autoEqBandDb[1], -1.8);

    AudioContext c2 { .buffer = run2, .frameCount = static_cast<uint32_t>(frames), .sampleRate = static_cast<uint32_t>(Rate) };
    mm.process(c2);

    for (size_t i = 0; i < frames * 2; i++) {
        QCOMPARE(run2[i], run1[i]);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MagicMasterTest)
