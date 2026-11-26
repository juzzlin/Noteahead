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

#include "true_peak_limiter_core_test.hpp"

#include "../../common/utils.hpp"
#include "../../domain/dsp/true_peak_limiter_core.hpp"
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/loudness_analyzer.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

} // namespace

void TruePeakLimiterCoreTest::test_nyquistQuarterHotSine_shouldRespectTruePeakCeiling()
{
    TruePeakLimiterCore core;
    core.setSampleRate(SampleRate);
    core.setThresholdDb(0.0);
    core.setCeilingDb(-1.2);
    core.setLookaheadMs(5.0);
    core.setAutoRelease(true);
    core.setBpm(140.0);

    DbTpMeter tpMeter;
    tpMeter.setSampleRate(SampleRate);

    LoudnessAnalyzer analyzer { static_cast<unsigned int>(SampleRate) };

    const size_t totalFrames { 24000 };
    std::vector<double> interleaved(totalFrames * 2, 0.0);
    // Amplitude +3.0 dBFS at fs/4 shifted by pi/4 produces high inter-sample peaks (~+6 dBTP raw)
    const double amplitude { static_cast<double>(Utils::Dsp::dbToLinear(3.0f)) };
    for (size_t i = 0; i < totalFrames; i++) {
        const double s { amplitude * std::sin(2.0 * std::numbers::pi * 0.25 * static_cast<double>(i) + std::numbers::pi * 0.25) };
        interleaved[i * 2] = s;
        interleaved[i * 2 + 1] = s;
    }

    core.processBlock(interleaved, static_cast<uint32_t>(totalFrames));

    std::vector<float> floatInterleaved(totalFrames * 2, 0.0f);
    for (size_t i = 0; i < totalFrames; i++) {
        double l { interleaved[i * 2] };
        double r { interleaved[i * 2 + 1] };
        tpMeter.processSample(l, r);
        floatInterleaved[i * 2] = static_cast<float>(l);
        floatInterleaved[i * 2 + 1] = static_cast<float>(r);
    }
    analyzer.process(floatInterleaved.data(), floatInterleaved.size());
    const auto analysisResult { analyzer.calculate() };

    QVERIFY2(tpMeter.truePeakHoldL() <= -1.2f + 0.01f, qPrintable(QString { "DbTpMeter peak %1" }.arg(tpMeter.truePeakHoldL())));
    QVERIFY2(analysisResult.truePeak <= -1.2f + 0.01f, qPrintable(QString { "Analyzer truePeak %1" }.arg(analysisResult.truePeak)));
    QVERIFY2(core.outputTruePeakHoldDb() <= -1.2f + 0.01f, qPrintable(QString { "Core truePeakHold %1" }.arg(core.outputTruePeakHoldDb())));
    QVERIFY(core.reductionDb() < -1.0f);
}

void TruePeakLimiterCoreTest::test_subThresholdSine_shouldPassAtUnityGain()
{
    TruePeakLimiterCore core;
    core.setSampleRate(SampleRate);
    core.setThresholdDb(0.0);
    core.setCeilingDb(-1.0);
    core.setLookaheadMs(5.0);

    const size_t latency { core.latencyFrames(SampleRate) };
    const size_t totalFrames { latency + 2000 };
    const double amp { 0.25 }; // ~ -12 dBFS
    std::vector<double> input(totalFrames, 0.0);
    std::vector<double> stereo(totalFrames * 2, 0.0);
    for (size_t i = 0; i < totalFrames; i++) {
        const double s { amp * std::sin(2.0 * std::numbers::pi * 100.0 * static_cast<double>(i) / SampleRate) };
        input[i] = s;
        stereo[i * 2] = s;
        stereo[i * 2 + 1] = -s;
    }

    core.processBlock(stereo, static_cast<uint32_t>(totalFrames));

    for (size_t i = latency; i < totalFrames; i++) {
        QVERIFY2(std::abs(stereo[i * 2] - input[i - latency]) < 1.0e-12, qPrintable(QString { "Frame %1 mismatch" }.arg(i)));
        QVERIFY2(std::abs(stereo[i * 2 + 1] + input[i - latency]) < 1.0e-12, qPrintable(QString { "Frame %1 R mismatch" }.arg(i)));
    }
    QCOMPARE(core.reductionDb(), 0.0f);
}

void TruePeakLimiterCoreTest::test_impulseDelay_shouldMatchLatencyFrames()
{
    for (const double lookaheadMs : { 0.0, 4.0, 5.0, 10.0 }) {
        TruePeakLimiterCore core;
        core.setSampleRate(SampleRate);
        core.setThresholdDb(0.0);
        core.setCeilingDb(-1.0);
        core.setLookaheadMs(lookaheadMs);

        const size_t expectedLatency { core.latencyFrames(SampleRate) };
        const size_t totalFrames { expectedLatency + 100 };
        std::vector<double> stereo(totalFrames * 2, 0.0);
        stereo[0] = 0.5;
        stereo[1] = 0.5;

        core.processBlock(stereo, static_cast<uint32_t>(totalFrames));

        size_t maxIdx { 0 };
        double maxVal { 0.0 };
        for (size_t i = 0; i < totalFrames; i++) {
            if (std::abs(stereo[i * 2]) > maxVal) {
                maxVal = std::abs(stereo[i * 2]);
                maxIdx = i;
            }
        }
        QCOMPARE(maxIdx, expectedLatency);
        QVERIFY(std::abs(maxVal - 0.5) < 1.0e-12);
    }
}

void TruePeakLimiterCoreTest::test_autoRelease_shouldRecoverFasterAtHigherBpm()
{
    const auto measureGainAfterRelease = [](double bpm, float & hotReduction) {
        TruePeakLimiterCore core;
        core.setSampleRate(SampleRate);
        core.setThresholdDb(-6.0);
        core.setCeilingDb(-1.0);
        core.setBoost(false);
        core.setLookaheadMs(4.0);
        core.setAutoRelease(true);
        core.setBpm(bpm);

        // Drive sustained hot audio (~0 dBFS) for 300 ms so sustained GR tracker engages, then drop to -18 dBFS
        const size_t hotFrames { 14400 };
        const size_t tailFrames { 4800 }; // 100 ms into release
        for (size_t i = 0; i < hotFrames; i++) {
            double l { 0.9 * std::sin(2.0 * std::numbers::pi * 100.0 * static_cast<double>(i) / SampleRate) };
            double r { l };
            core.processSample(l, r);
        }
        hotReduction = core.reductionDb();

        for (size_t i = 0; i < tailFrames; i++) {
            double l { 0.12 * std::sin(2.0 * std::numbers::pi * 100.0 * static_cast<double>(i) / SampleRate) };
            double r { l };
            core.processSample(l, r);
        }
        return core.reductionDb();
    };

    float hotSlow { 0.0f };
    float hotFast { 0.0f };
    const float grSlowBpm { measureGainAfterRelease(90.0, hotSlow) };
    const float grFastBpm { measureGainAfterRelease(170.0, hotFast) };
    QVERIFY(hotSlow < -3.0f);
    QVERIFY(hotFast < -3.0f);
    // Faster BPM => closer to 0.0 dB (less remaining reduction after 100 ms)
    QVERIFY2(grFastBpm > grSlowBpm + 0.5f, qPrintable(QString { "170 BPM GR %1 vs 90 BPM GR %2" }.arg(grFastBpm).arg(grSlowBpm)));
}

void TruePeakLimiterCoreTest::test_reset_shouldBeBitIdentical()
{
    TruePeakLimiterCore core;
    core.setSampleRate(SampleRate);
    core.setThresholdDb(-6.0);
    core.setCeilingDb(-1.0);
    core.setLookaheadMs(5.0);
    core.setBoost(true);
    core.setAutoRelease(true);
    core.setBpm(138.0);

    const size_t frames { 4096 };
    std::vector<double> run1(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double s { 0.8 * std::sin(2.0 * std::numbers::pi * 250.0 * static_cast<double>(i) / SampleRate) };
        run1[i * 2] = s;
        run1[i * 2 + 1] = -0.5 * s;
    }
    auto run2 = run1;

    core.processBlock(run1, static_cast<uint32_t>(frames));
    core.reset();
    core.processBlock(run2, static_cast<uint32_t>(frames));

    for (size_t i = 0; i < frames * 2; i++) {
        QCOMPARE(run2[i], run1[i]);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TruePeakLimiterCoreTest)
