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
#include <memory>
#include <numbers>
#include <random>
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

//! An analyzer in @p bandCountMode, run far enough to have adopted it.
//!
//! The mode only reaches the bands when the audio thread next runs, so anything asked of the
//! analyzer before that describes the mode it is leaving.
static std::unique_ptr<Rta> makeRta(float bandCountMode)
{
    auto rta = std::make_unique<Rta>();
    rta->setAnalysisEnabled(true);
    if (auto p = rta->parameter(Constants::NahdXml::xmlKeyBandCount().toStdString()); p) {
        p->get().update(bandCountMode);
    }
    rta->sync();
    auto warmup = makeSilentBuffer(64);
    feedBuffer(*rta, warmup);
    return rta;
}

//! Index of the bar covering @p freq, from the normalised log-frequency spans the analyzer draws at.
static size_t bandAt(const Rta & rta, double freq)
{
    const auto positions = rta.bandLogPositions();
    const double x = std::log10(freq / 20.0) / std::log10(1000.0);
    for (size_t i = 0; i < positions.size(); i++) {
        if (x >= positions[i].first && x < positions[i].second) {
            return i;
        }
    }
    return 0;
}

static void feedTone(Rta & rta, double freq, double & phase, uint32_t frames)
{
    std::vector<double> buf(frames * 2);
    for (uint32_t i = 0; i < frames; i++) {
        const double s = 0.5 * std::sin(phase);
        phase += 2.0 * std::numbers::pi * freq / DefaultSampleRate;
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;
    }
    feedBuffer(rta, buf);
}

//! Milliseconds a tone at @p freq takes to bring its own bar within 3 dB of where it settles.
static double measureResponseMs(double freq)
{
    constexpr uint32_t blockFrames = 64;

    auto settled = makeRta(2.0f);
    const auto band = bandAt(*settled, freq);
    double phase = 0.0;
    for (uint32_t i = 0; i < DefaultSampleRate * 3 / blockFrames; i++) {
        feedTone(*settled, freq, phase, blockFrames);
    }
    const double steadyDb = settled->bandMagnitudesDb()[band];

    auto fresh = makeRta(2.0f);
    for (uint32_t i = 0; i < DefaultSampleRate * 2 / blockFrames; i++) {
        auto silence = makeSilentBuffer(blockFrames);
        feedBuffer(*fresh, silence);
    }

    phase = 0.0;
    uint32_t blocks = 0;
    const uint32_t maxBlocks = DefaultSampleRate * 3 / blockFrames;
    for (uint32_t i = 0; i < maxBlocks; i++) {
        feedTone(*fresh, freq, phase, blockFrames);
        blocks++;
        if (fresh->bandMagnitudesDb()[band] > steadyDb - 3.0) {
            break;
        }
    }

    return 1000.0 * blocks * blockFrames / DefaultSampleRate;
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

void RtaTest::test_bandCount_mode96_shouldProduceNinetySixBars()
{
    // 96 is the mode that was appended after 128, so this is also what says the stored value still
    // means what it meant: a project saved in 128-band mode must not come back as 96.
    QCOMPARE(makeRta(3.0f)->bandMagnitudesDb().size(), size_t { 96 });
    QCOMPARE(makeRta(2.0f)->bandMagnitudesDb().size(), size_t { 128 });
}

void RtaTest::test_response_highBand_shouldSettleSoonerThanLowBand()
{
    // The point of analysing each band at its own resolution. A bar at 8 kHz is four hundred times
    // wider than one at 30 Hz and has no use for the window the bottom of the display needs, so it
    // must not be made to wait for it. The low bar is measured too, because the bound that matters
    // is the ratio: the bottom is slow for reasons no analyzer can argue with.
    const double highMs = measureResponseMs(8000.0);
    const double lowMs = measureResponseMs(120.0);

    QVERIFY2(highMs < 40.0, qPrintable(QString("8 kHz took %1 ms").arg(highMs)));
    QVERIFY2(highMs * 4.0 < lowMs, qPrintable(QString("8 kHz took %1 ms against %2 ms at 120 Hz").arg(highMs).arg(lowMs)));
}

void RtaTest::test_bands_whiteNoise_shouldNotStepWhereResolutionChanges()
{
    auto rta = makeRta(2.0f);

    std::mt19937 rng { 4242 };
    std::uniform_real_distribution<double> dist { -0.3, 0.3 };
    constexpr uint32_t blockFrames = 256;

    // Averaged over time as well as over bins: one reading of a band holding a couple of bins of
    // noise varies by several dB on its own, which is not what this is looking for.
    std::vector<double> sums(rta->bandMagnitudesDb().size(), 0.0);
    int readings = 0;
    for (uint32_t i = 0; i < DefaultSampleRate * 6 / blockFrames; i++) {
        std::vector<double> buf(blockFrames * 2);
        for (uint32_t j = 0; j < blockFrames; j++) {
            const double s = dist(rng);
            buf[j * 2] = s;
            buf[j * 2 + 1] = s;
        }
        feedBuffer(*rta, buf);
        if (i > DefaultSampleRate / blockFrames) {
            const auto levels = rta->bandMagnitudesDb();
            for (size_t b = 0; b < sums.size(); b++) {
                sums[b] += levels[b];
            }
            readings++;
        }
    }

    // Noise of one colour must draw one curve. Neighbouring bars are measured by different windows
    // wherever the resolution changes, and a level that did not survive that change would draw a
    // step at the crossover rather than the even rise log-spaced bands give white noise. Bars below
    // a few hundred hertz hold a single bin, where one reading differs from the next for reasons
    // that have nothing to do with which window produced it.
    const auto positions = rta->bandLogPositions();
    for (size_t i = 1; i < sums.size(); i++) {
        if (positions[i].first < std::log10(300.0 / 20.0) / std::log10(1000.0)) {
            continue;
        }
        const double step = std::abs(sums[i] - sums[i - 1]) / readings;
        QVERIFY2(step < 2.5, qPrintable(QString("bars %1 and %2 are %3 dB apart").arg(i - 1).arg(i).arg(step)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RtaTest)
