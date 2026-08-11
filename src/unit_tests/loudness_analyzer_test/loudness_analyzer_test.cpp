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

#include "loudness_analyzer_test.hpp"

#include "../../domain/utility/loudness_analyzer.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

static constexpr double sampleRate { 48000.0 };

static void feedSine(LoudnessAnalyzer & analyzer, double freq, double amplitude, double seconds)
{
    const int totalSamples { static_cast<int>(sampleRate * seconds) };
    std::vector<float> buffer(4096 * 2);
    int sampleIdx { 0 };

    for (int i { 0 }; i < totalSamples; i++) {
        const double s { amplitude * std::sin(2.0 * std::numbers::pi * freq / sampleRate * i) };
        buffer[sampleIdx++] = static_cast<float>(s);
        buffer[sampleIdx++] = static_cast<float>(s);

        if (sampleIdx >= static_cast<int>(buffer.size())) {
            analyzer.process(buffer.data(), buffer.size());
            sampleIdx = 0;
        }
    }
    if (sampleIdx > 0) {
        analyzer.process(buffer.data(), sampleIdx);
    }
}

void LoudnessAnalyzerTest::test_loudness_sine_shouldMeasureIntegratedLoudness()
{
    LoudnessAnalyzer analyzer { sampleRate };

    // Expected loudness for 0.1 amplitude 1kHz sine:
    // L ≈ -0.691 + 10*log10(2 * 0.01/2) ≈ -20.7 LUFS
    feedSine(analyzer, 1000.0, 0.1, 4.0);

    const auto result = analyzer.calculate();

    // Verify integrated loudness is within a small tolerance (allowing for block alignment/K-filtering deviations)
    QVERIFY(result.integratedLoudness > -22.0f);
    QVERIFY(result.integratedLoudness < -19.0f);

    // Gated threshold should be -10 dB relative to absolute gated average loudness, i.e. around -30.7 LUFS
    QVERIFY(result.threshold > -32.0f);
    QVERIFY(result.threshold < -29.0f);
}

void LoudnessAnalyzerTest::test_loudness_varying_shouldMeasureLRA()
{
    LoudnessAnalyzer analyzer { sampleRate };

    // Feed a louder sine for 4 seconds (loudness ~ -14 LUFS)
    feedSine(analyzer, 1000.0, 0.2, 4.0);
    // Feed a quieter sine for 4 seconds (loudness ~ -34 LUFS)
    feedSine(analyzer, 1000.0, 0.02, 4.0);

    const auto result = analyzer.calculate();

    // Since the volume shifts significantly, LRA should be > 0.0 LU
    QVERIFY(result.loudnessRange > 10.0f);
    QVERIFY(result.loudnessRange < 25.0f);
}

void LoudnessAnalyzerTest::test_loudness_truePeak()
{
    LoudnessAnalyzer analyzer { sampleRate };

    // Feed a sine wave peaking at exactly 0.5 linear amplitude
    feedSine(analyzer, 1000.0, 0.5, 1.0);

    const auto result = analyzer.calculate();

    // 0.5 linear amplitude corresponds to 20*log10(0.5) ≈ -6.02 dBTP
    QVERIFY(result.truePeak > -6.5f);
    QVERIFY(result.truePeak < -5.5f);
}

namespace {

//! Loud passage, then one 14 dB below it, then an optional stretch of digital silence. The quiet
//! passage sits just under the relative threshold the first two produce, so it is what the gating
//! has to get right; the silence must have no say in the matter at all.
LoudnessAnalyzer::Result analyzeWithTail(double silenceSeconds)
{
    LoudnessAnalyzer analyzer { sampleRate };
    feedSine(analyzer, 1000.0, 0.1, 20.0);
    feedSine(analyzer, 1000.0, 0.02, 20.0);
    if (silenceSeconds > 0.0) {
        feedSine(analyzer, 1000.0, 0.0, silenceSeconds);
    }
    return analyzer.calculate();
}

} // namespace

void LoudnessAnalyzerTest::test_loudness_trailingSilence_shouldNotChangeIntegratedLoudness()
{
    // Silence is below the absolute gate, so however much of it a song trails off into, the loudness
    // of the programme material before it must read exactly the same.
    const auto noTail = analyzeWithTail(0.0);
    QVERIFY(noTail.integratedLoudness > -21.0f);
    QVERIFY(noTail.integratedLoudness < -19.0f);

    for (const auto seconds : { 5.0, 20.0, 60.0 }) {
        const auto withTail = analyzeWithTail(seconds);
        QVERIFY2(std::abs(withTail.integratedLoudness - noTail.integratedLoudness) < 0.01f,
                 qPrintable(QString { "%1 s of silence moved the reading from %2 to %3" }
                              .arg(seconds)
                              .arg(noTail.integratedLoudness)
                              .arg(withTail.integratedLoudness)));
    }
}

void LoudnessAnalyzerTest::test_loudness_trailingSilence_shouldNotChangeThreshold()
{
    // The relative threshold is what the silence used to drag down, letting quiet passages that
    // belong outside the measurement back into it: 60 s of it used to move this by over 4 LU.
    const auto noTail = analyzeWithTail(0.0);
    const auto shortTail = analyzeWithTail(5.0);

    // However much silence follows, the threshold must not move a further step: past the edge every
    // block is discarded outright, so there is nothing left for a longer tail to change.
    QCOMPARE(analyzeWithTail(20.0).threshold, shortTail.threshold);
    QCOMPARE(analyzeWithTail(60.0).threshold, shortTail.threshold);

    // The edge itself does count, since those gating blocks are part programme material, but it is
    // worth hundredths of a dB rather than the whole tail's worth.
    QVERIFY(std::abs(shortTail.threshold - noTail.threshold) < 0.1f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LoudnessAnalyzerTest)
