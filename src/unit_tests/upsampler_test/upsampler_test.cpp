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

#include "upsampler_test.hpp"
#include "../../domain/dsp/upsampler.hpp"

#include <QTest>

#include <array>
#include <cmath>
#include <numbers>
#include <random>

namespace noteahead {

namespace {

constexpr double BaseSampleRate { 48000.0 };

// Peak amplitude of a low-frequency sine after an upsample -> (identity) -> downsample round trip.
float roundTripPeak(uint8_t factor)
{
    Upsampler up;
    Decimator down;
    std::array<float, 4> high {};
    float peak = 0.0f;
    const int samples = 2000;
    for (int n = 0; n < samples; n++) {
        const float x = 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * static_cast<float>(n) / 64.0f);
        up.process(x, high.data(), factor);
        const float y = down.process(high.data(), factor);
        if (n > 64) { // Skip filter warmup
            peak = std::max(peak, std::abs(y));
        }
    }
    return peak;
}
} // namespace

void UpsamplerTest::test_process_factorOne_shouldPassThrough()
{
    Upsampler up;
    std::array<float, 4> out {};
    up.process(0.33f, out.data(), 1);
    QCOMPARE(out[0], 0.33f);
}

void UpsamplerTest::test_process_dcInput_shouldPreserveLevel()
{
    Upsampler2x up;
    float a = 0.0f;
    float b = 0.0f;
    for (int i = 0; i < 64; i++) {
        up.process(1.0f, a, b);
    }
    // Each polyphase branch has unity DC gain, so both output phases must reproduce the DC level.
    QVERIFY(std::abs(a - 1.0f) < 0.001f);
    QVERIFY(std::abs(b - 1.0f) < 0.001f);
}

void UpsamplerTest::test_roundTrip_factorTwo_lowFrequency_shouldReconstruct()
{
    // A low-frequency sine of amplitude 0.5 must survive the round trip with its level intact.
    QVERIFY(std::abs(roundTripPeak(2) - 0.5f) < 0.02f);
}

void UpsamplerTest::test_roundTrip_factorFour_lowFrequency_shouldReconstruct()
{
    QVERIFY(std::abs(roundTripPeak(4) - 0.5f) < 0.02f);
}

void UpsamplerTest::test_clampOversampleFactor_shouldAllowSupportedValues()
{
    QCOMPARE(clampOversampleFactor(1), static_cast<uint8_t>(1));
    QCOMPARE(clampOversampleFactor(2), static_cast<uint8_t>(2));
    QCOMPARE(clampOversampleFactor(4), static_cast<uint8_t>(4));
}

void UpsamplerTest::test_clampOversampleFactor_shouldFallBackToTwo()
{
    QCOMPARE(clampOversampleFactor(0), static_cast<uint8_t>(2));
    QCOMPARE(clampOversampleFactor(3), static_cast<uint8_t>(2));
    QCOMPARE(clampOversampleFactor(8), static_cast<uint8_t>(2));
}

void UpsamplerTest::test_decimator_factorOne_shouldPassThrough()
{
    Decimator decimator;
    const std::array<float, 4> samples { 0.42f, -1.0f, 0.0f, 0.0f };
    QCOMPARE(decimator.process(samples.data(), 1), 0.42f);
}

void UpsamplerTest::test_decimator_dcInput_shouldPreserveLevel()
{
    const std::array<float, 4> dc { 1.0f, 1.0f, 1.0f, 1.0f };
    for (const uint8_t factor : { 2, 4 }) {
        Decimator decimator;
        float out = 0.0f;
        for (int i = 0; i < 128; i++) {
            out = decimator.process(dc.data(), factor);
        }
        QVERIFY2(std::abs(out - 1.0f) < 0.001f, qPrintable(QString { "factor %1 gave %2" }.arg(factor).arg(out)));
    }
}

void UpsamplerTest::test_decimator_audioBand_shouldNotAttenuateWithOversampling()
{
    // The regression this guards: a decimation filter that droops inside the audio band makes
    // oversampled playback and export duller than 1x, and 4x duller still, because the cascade
    // applies the droop twice.
    const auto levelAt = [](double frequency, uint8_t factor) {
        Decimator decimator;
        const double highRate = BaseSampleRate * factor;
        std::array<float, 4> high {};
        double squareSum = 0.0;
        int counted = 0;
        for (int n = 0; n < 8000; n++) {
            for (uint8_t k = 0; k < factor; k++) {
                const double t = static_cast<double>(n) * factor + k;
                high[k] = static_cast<float>(0.5 * std::sin(2.0 * std::numbers::pi * frequency * t / highRate));
            }
            const double out = decimator.process(high.data(), factor);
            if (n > 200) { // Past the filter's warm-up
                squareSum += out * out;
                counted++;
            }
        }
        return std::sqrt(squareSum / counted);
    };

    for (const double frequency : { 1000.0, 5000.0, 10000.0, 15000.0 }) {
        const double reference = levelAt(frequency, 1);
        for (const uint8_t factor : { 2, 4 }) {
            const double decibels = 20.0 * std::log10(levelAt(frequency, factor) / reference);
            QVERIFY2(std::abs(decibels) < 0.1,
                     qPrintable(QString { "%1 Hz at %2x: %3 dB" }.arg(frequency).arg(factor).arg(decibels)));
        }
    }
}

void UpsamplerTest::test_decimator_aboveBaseNyquist_shouldRejectAliases()
{
    // Content above the base Nyquist folds back into the audio band, which is the entire reason
    // for filtering before decimating.
    Decimator decimator;
    constexpr double frequency { 36000.0 }; // Folds to 12 kHz at a 48 kHz base rate
    std::array<float, 4> high {};
    double squareSum = 0.0;
    int counted = 0;
    for (int n = 0; n < 8000; n++) {
        for (uint8_t k = 0; k < 2; k++) {
            const double t = static_cast<double>(n) * 2 + k;
            high[k] = static_cast<float>(0.5 * std::sin(2.0 * std::numbers::pi * frequency * t / (BaseSampleRate * 2)));
        }
        const double out = decimator.process(high.data(), 2);
        if (n > 200) {
            squareSum += out * out;
            counted++;
        }
    }

    const double rms = std::sqrt(squareSum / counted);
    const double decibels = 20.0 * std::log10(rms / (0.5 / std::numbers::sqrt2));
    QVERIFY2(decibels < -60.0, qPrintable(QString { "alias only %1 dB down" }.arg(decibels)));
}

void UpsamplerTest::test_noiseGain_shouldKeepInBandNoiseLevelConstant()
{
    // White noise drawn one sample per clock spreads a fixed power up to Nyquist, so rendering it
    // at an oversampled rate leaves less of it below the base Nyquist. Without the compensation a
    // noisy patch — hi-hats, snares, a synth noise source — gets quieter the more you oversample.
    const auto inBandLevel = [](uint8_t factor, bool compensate) {
        std::mt19937 rng { 1234 };
        std::uniform_real_distribution<float> dist { -1.0f, 1.0f };
        const float gain = compensate ? noiseGainForOversampling(factor) : 1.0f;
        Decimator decimator;
        std::array<float, 4> high {};
        double squareSum = 0.0;
        int counted = 0;
        for (int n = 0; n < 60000; n++) {
            for (uint8_t k = 0; k < factor; k++) {
                high[k] = dist(rng) * gain;
            }
            const double out = decimator.process(high.data(), factor);
            if (n > 500) {
                squareSum += out * out;
                counted++;
            }
        }
        return std::sqrt(squareSum / counted);
    };

    const double reference = inBandLevel(1, true);
    for (const uint8_t factor : { 2, 4 }) {
        const double uncompensated = 20.0 * std::log10(inBandLevel(factor, false) / reference);
        const double compensated = 20.0 * std::log10(inBandLevel(factor, true) / reference);
        // Uncompensated, the level falls by 10*log10(factor): 3 dB at 2x, 6 dB at 4x.
        QVERIFY2(uncompensated < -2.0, qPrintable(QString { "%1x uncompensated only %2 dB" }.arg(factor).arg(uncompensated)));
        QVERIFY2(std::abs(compensated) < 0.5, qPrintable(QString { "%1x compensated %2 dB" }.arg(factor).arg(compensated)));
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::UpsamplerTest)
