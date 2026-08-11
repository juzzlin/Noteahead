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

#include "lufs_meter_test.hpp"

#include "../../domain/utility/loudness_analyzer.hpp"
#include "../../domain/utility/lufs_meter.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

static constexpr double sampleRate = 48000.0;
static constexpr float lufsFloor = -70.0f;

// Feed silence for the given number of seconds and fill the ring buffer.
static void feedSilence(LufsMeter & meter, double seconds)
{
    const int samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        double l = 0.0, r = 0.0;
        meter.process(l, r);
    }
}

// Feed a stereo sine for the given number of seconds.
static void feedSine(LufsMeter & meter, double freq, double amplitude, double seconds)
{
    const int samples = static_cast<int>(sampleRate * seconds);
    for (int i = 0; i < samples; i++) {
        const double s = amplitude * std::sin(2.0 * std::numbers::pi * freq / sampleRate * i);
        double l = s, r = s;
        meter.process(l, r);
    }
}

// Interleaved stereo sine, so that the streaming meter and the offline analyzer can be fed the very
// same samples rather than two separately generated signals.
static std::vector<float> makeSine(double freq, double amplitude, double seconds, int phaseOffset = 0)
{
    const auto frames = static_cast<size_t>(sampleRate * seconds);
    std::vector<float> data(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        const auto s = static_cast<float>(amplitude * std::sin(2.0 * std::numbers::pi * freq / sampleRate * static_cast<double>(i + phaseOffset)));
        data[i * 2] = s;
        data[i * 2 + 1] = s;
    }
    return data;
}

static void feed(LufsMeter & meter, const std::vector<float> & interleaved)
{
    for (size_t i = 0; i < interleaved.size(); i += 2) {
        double l = interleaved[i];
        double r = interleaved[i + 1];
        meter.process(l, r);
    }
}

void LufsMeterTest::test_lufsMeter_silence_shouldReturnFloor()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    // Before any audio: both readings are at the floor
    QCOMPARE(meter.momentaryLufs(), lufsFloor);
    QCOMPARE(meter.shortTermLufs(), lufsFloor);

    feedSilence(meter, 3.0);

    QCOMPARE(meter.momentaryLufs(), lufsFloor);
    QCOMPARE(meter.shortTermLufs(), lufsFloor);
}

void LufsMeterTest::test_lufsMeter_sine_shouldMeasureLoudness()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    // Amplitude 0.1 at 1kHz. K-weighting barely changes 1kHz so expected loudness:
    // L ≈ -0.691 + 10*log10(2 * 0.01/2) = -0.691 + 10*log10(0.01) ≈ -20.7 LUFS
    feedSine(meter, 1000.0, 0.1, 3.5);

    // Allow ±3 dB tolerance for K-weighting and block-boundary effects
    QVERIFY(meter.shortTermLufs() > -24.0f);
    QVERIFY(meter.shortTermLufs() < -18.0f);

    // Momentary is also active
    QVERIFY(meter.momentaryLufs() > -24.0f);
    QVERIFY(meter.momentaryLufs() < -18.0f);
}

void LufsMeterTest::test_lufsMeter_passThrough_shouldNotModifySignal()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    // The meter must not alter the audio stream
    for (int i = 0; i < 1000; i++) {
        const double origL = std::sin(2.0 * std::numbers::pi * 440.0 / sampleRate * i);
        const double origR = std::cos(2.0 * std::numbers::pi * 440.0 / sampleRate * i);
        double l = origL;
        double r = origR;
        meter.process(l, r);
        QCOMPARE(l, origL);
        QCOMPARE(r, origR);
    }
}

void LufsMeterTest::test_lufsMeter_reset_shouldClearReadings()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    feedSine(meter, 1000.0, 0.5, 3.5);

    // Confirm something is being measured before reset
    QVERIFY(meter.shortTermLufs() > lufsFloor);

    meter.reset();

    QCOMPARE(meter.momentaryLufs(), lufsFloor);
    QCOMPARE(meter.shortTermLufs(), lufsFloor);
}

void LufsMeterTest::test_lufsMeter_sampleRateChange_shouldReinitialize()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    feedSine(meter, 1000.0, 0.5, 3.5);
    QVERIFY(meter.shortTermLufs() > lufsFloor);

    // Changing sample rate resets state and updates block size
    meter.setSampleRate(44100.0);
    feedSine(meter, 1000.0, 0.001, 1.0);

    // Should now be measuring the quiet signal, not the old loud one
    QVERIFY(meter.momentaryLufs() < -30.0f);
}

void LufsMeterTest::test_lufsMeter_integrated_steadyTone_shouldMatchOfflineAnalyzer()
{
    // LoudnessAnalyzer is the reference: it is what the audio export reports, and it does the gating
    // offline over the whole signal. The streaming meter has to arrive at the same number from a
    // histogram, so pin one against the other on identical samples.
    const auto loud = makeSine(1000.0, 0.1, 5.0);
    const auto quiet = makeSine(1000.0, 0.03, 5.0);

    LufsMeter meter;
    meter.setSampleRate(sampleRate);
    LoudnessAnalyzer analyzer { sampleRate };

    for (const auto & part : { loud, quiet }) {
        feed(meter, part);
        analyzer.process(part.data(), part.size());
    }

    const auto reference = analyzer.calculate().integratedLoudness;
    QVERIFY(reference > -30.0f);
    QVERIFY(std::abs(meter.integratedLufs() - reference) < 0.15f);
}

void LufsMeterTest::test_lufsMeter_integrated_afterSilence_shouldIgnoreTheSilence()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    feed(meter, makeSine(1000.0, 0.1, 5.0));
    const auto afterTone = meter.integratedLufs();
    QVERIFY(afterTone > -24.0f);
    QVERIFY(afterTone < -18.0f);

    feedSilence(meter, 5.0);
    const auto afterSilence = meter.integratedLufs();

    // The three gating blocks straddling the tone-to-silence edge are part tone and part silence, so
    // they are quieter than the tone and do count. That is worth a couple of tenths, no more; an
    // ungated mean would be somewhere near -25 by now.
    QVERIFY(std::abs(afterSilence - afterTone) < 0.25f);

    // Past the edge, though, every block is pure silence and the absolute gate discards it outright:
    // however long it runs, the reading must not move at all.
    feedSilence(meter, 20.0);
    QCOMPARE(meter.integratedLufs(), afterSilence);

    // The momentary and short-term windows do follow the silence down, unlike the integrated one.
    QCOMPARE(meter.momentaryLufs(), lufsFloor);
}

void LufsMeterTest::test_lufsMeter_integrated_afterQuietPassage_shouldFollowTheRelativeGate()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    feed(meter, makeSine(1000.0, 0.1, 5.0));
    const auto afterLoud = meter.integratedLufs();

    // 26 dB down: above the absolute gate, so it is measured, but far enough below the relative
    // threshold that it must not pull the integrated reading towards it.
    feed(meter, makeSine(1000.0, 0.005, 5.0));

    QVERIFY(meter.shortTermLufs() < -40.0f);
    QVERIFY(std::abs(meter.integratedLufs() - afterLoud) < 0.5f);
}

void LufsMeterTest::test_lufsMeter_integrated_shortAudio_shouldStayAtFloor()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    // A gating block is 400 ms. Below that there is nothing to gate, so integrated stays unset even
    // though the momentary reading already has something to show.
    feed(meter, makeSine(1000.0, 0.1, 0.35));

    QCOMPARE(meter.integratedLufs(), lufsFloor);
    QVERIFY(meter.momentaryLufs() > lufsFloor);
}

void LufsMeterTest::test_lufsMeter_requestReset_shouldClearReadingsAtNextSample()
{
    LufsMeter meter;
    meter.setSampleRate(sampleRate);

    feed(meter, makeSine(1000.0, 0.1, 5.0));
    QVERIFY(meter.integratedLufs() > -24.0f);

    meter.requestReset();

    // The readings blank at once, without waiting for the audio thread to come back round.
    QCOMPARE(meter.momentaryLufs(), lufsFloor);
    QCOMPARE(meter.shortTermLufs(), lufsFloor);
    QCOMPARE(meter.integratedLufs(), lufsFloor);

    // ...and the accumulated state really is gone: what follows measures on its own terms.
    feed(meter, makeSine(1000.0, 0.01, 5.0));

    QVERIFY(meter.integratedLufs() < -35.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LufsMeterTest)
