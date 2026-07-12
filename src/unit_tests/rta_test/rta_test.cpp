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

#include "rta_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/utility/rta.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

static constexpr uint32_t DefaultSampleRate = 44100;

static std::vector<double> makeSilentBuffer(uint32_t frames)
{
    return std::vector<double>(frames * 2, 0.0);
}

static std::vector<double> makeSineBuffer(uint32_t frames, double freq, double amplitude)
{
    std::vector<double> buf(frames * 2);
    for (uint32_t i = 0; i < frames; i++) {
        const double s = amplitude * std::sin(2.0 * std::numbers::pi * freq / DefaultSampleRate * i);
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;
    }
    return buf;
}

static void feedBuffer(Rta & rta, std::vector<double> & buf, uint32_t sampleRate = DefaultSampleRate)
{
    AudioContext ctx;
    ctx.buffer = buf;
    ctx.frameCount = static_cast<uint32_t>(buf.size() / 2);
    ctx.sampleRate = sampleRate;
    rta.process(ctx);
}

void RtaTest::test_typeId_shouldReturnExpectedString()
{
    Rta rta;
    QCOMPARE(rta.typeId(), std::string { "b9f2e4d7-3a8c-4e6b-9f1d-2c5e7a0b3d4f" });
}

void RtaTest::test_type_shouldReturnExpectedString()
{
    Rta rta;
    QCOMPARE(rta.type(), Constants::RackEffectType::rta().toStdString());
}

void RtaTest::test_process_simple_shouldBePassthrough()
{
    Rta rta;
    double l = 0.75;
    double r = -0.25;
    rta.process(l, r);
    QCOMPARE(l, 0.75);
    QCOMPARE(r, -0.25);
}

void RtaTest::test_bandMagnitudesDb_afterConstruction_shouldBeNonEmpty()
{
    Rta rta;
    QVERIFY(!rta.bandMagnitudesDb().empty());
}

void RtaTest::test_bandLogPositions_sizeMatchesBandCount()
{
    Rta rta;
    QCOMPARE(rta.bandLogPositions().size(), rta.bandMagnitudesDb().size());
}

void RtaTest::test_process_audioContext_analysisDisabled_shouldNotUpdateBands()
{
    Rta rta;
    // analysis is disabled by default
    const auto initialMags = rta.bandMagnitudesDb();

    auto buf = makeSineBuffer(4096, 1000.0, 0.8);
    feedBuffer(rta, buf);

    QCOMPARE(rta.bandMagnitudesDb(), initialMags);
}

void RtaTest::test_process_audioContext_analysisEnabled_silence_shouldGoLowAfterAnalysis()
{
    Rta rta;
    rta.setAnalysisEnabled(true);

    // Feed enough silence to trigger multiple analysis hops (fast hop = 128 @ 44100 Hz in default mode)
    auto buf = makeSilentBuffer(8192);
    feedBuffer(rta, buf);

    const auto mags = rta.bandMagnitudesDb();
    const float maxMag = *std::max_element(mags.begin(), mags.end());
    // Silence drives smoothed power to zero; 10*log10(1e-20) ≈ -200 dBFS
    QVERIFY(maxMag < -50.0f);
}

void RtaTest::test_process_audioContext_analysisEnabled_sine1kHz_shouldDetectEnergy()
{
    Rta rta;
    rta.setAnalysisEnabled(true);

    // Feed a loud 1 kHz sine — well above the crossover (400 Hz) so the fast FFT handles it
    // Feed multiple hops to let the smoother converge
    for (int pass = 0; pass < 10; pass++) {
        auto buf = makeSineBuffer(1024, 1000.0, 0.5);
        feedBuffer(rta, buf);
    }

    const auto mags = rta.bandMagnitudesDb();
    const float maxMag = *std::max_element(mags.begin(), mags.end());
    // With a 0.5-amplitude sine the peak band should be well above -30 dBFS
    QVERIFY(maxMag > -30.0f);
}

void RtaTest::test_reset_shouldClearBandMagnitudesToFloor()
{
    Rta rta;
    rta.setAnalysisEnabled(true);

    // Drive the bands up
    for (int pass = 0; pass < 10; pass++) {
        auto buf = makeSineBuffer(1024, 1000.0, 0.5);
        feedBuffer(rta, buf);
    }

    const auto magsBeforeReset = rta.bandMagnitudesDb();
    const float maxBefore = *std::max_element(magsBeforeReset.begin(), magsBeforeReset.end());
    QVERIFY(maxBefore > -50.0f);

    rta.reset();

    const auto magsAfterReset = rta.bandMagnitudesDb();
    for (const float db : magsAfterReset) {
        QCOMPARE(db, -100.0f);
    }
}

void RtaTest::test_bandCount_mode64_shouldIncreaseBandCount()
{
    Rta rta;
    const int initialCount = static_cast<int>(rta.bandMagnitudesDb().size());
    QVERIFY(initialCount > 0);

    // Switch to 64-band mode
    if (auto p = rta.parameter(Constants::NahdXml::xmlKeyBandCount().toStdString()); p) {
        p->get().update(1.0f);
    }
    rta.sync();

    // Process one frame with analysis enabled to trigger syncParameters → buildBands
    rta.setAnalysisEnabled(true);
    auto buf = makeSilentBuffer(1);
    feedBuffer(rta, buf);

    const int newCount = static_cast<int>(rta.bandMagnitudesDb().size());
    QVERIFY(newCount > initialCount);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RtaTest)
