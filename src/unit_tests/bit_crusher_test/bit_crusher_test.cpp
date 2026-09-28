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

#include "bit_crusher_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/bit_crusher.hpp"
#include "../../domain/effects/effect.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

//! Runs a block through the effect and hands the buffer back.
std::vector<double> render(Effect & effect, const std::vector<double> & input, double bpm = 120.0)
{
    std::vector<double> buffer = input;
    AudioContext context { std::span<double>(buffer.data(), buffer.size()), static_cast<uint32_t>(buffer.size() / 2), static_cast<uint32_t>(SampleRate), bpm, {}, 1, false };
    effect.process(context);
    return buffer;
}

//! A stereo sine at one amplitude, as interleaved frames.
std::vector<double> tone(double frequency, double amplitude, size_t frames)
{
    std::vector<double> buffer(frames * 2, 0.0);
    for (size_t i = 0; i < frames; i++) {
        const double sample = amplitude * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate);
        buffer[i * 2] = sample;
        buffer[i * 2 + 1] = sample;
    }
    return buffer;
}

double peak(const std::vector<double> & buffer)
{
    double value = 0.0;
    for (auto && sample : buffer) {
        value = std::max(value, std::abs(sample));
    }
    return value;
}

void setParameter(Effect & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

} // namespace

void BitCrusherTest::test_defaults_shouldBeTransparent()
{
    BitCrusher effect;
    effect.sync();
    const auto input = tone(440.0, 0.5, 2048);
    const auto output = render(effect, input);
    // Full word length and a rate above the engine's: nothing is quantised and nothing is held, so
    // an effect just dropped into a rack must be inaudible until it is turned to something.
    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY2(std::abs(output[i] - input[i]) < 1e-9, qPrintable(QString::number(i)));
    }
}

void BitCrusherTest::test_bitDepth_shouldQuantizeToItsSteps()
{
    BitCrusher effect;
    setParameter(effect, Constants::NahdXml::xmlKeyBitDepth(), 4.0f);
    const auto output = render(effect, tone(440.0, 0.9, 4096));

    // Four bits is eight steps either side of zero, so every sample must land on one of them.
    constexpr double step = 1.0 / 8.0;
    for (auto && sample : output) {
        const double distance = std::abs(sample / step - std::round(sample / step));
        QVERIFY2(distance < 1e-9, qPrintable(QString::number(sample)));
    }
}

void BitCrusherTest::test_bitDepth_shouldKeepSilenceSilent()
{
    BitCrusher effect;
    setParameter(effect, Constants::NahdXml::xmlKeyBitDepth(), 2.0f);
    const auto output = render(effect, std::vector<double>(2048, 0.0));
    // The steps sit either side of zero rather than across the range, so nothing is put where
    // nothing was. Quantising the range instead leaves silence dithering between two levels.
    QCOMPARE(peak(output), 0.0);
}

void BitCrusherTest::test_rate_shouldHoldEachSample()
{
    BitCrusher effect;
    // A quarter of the sample rate, so each sample is held for four frames.
    setParameter(effect, Constants::NahdXml::xmlKeySampleRate(), 0.77545f);
    const auto output = render(effect, tone(1000.0, 0.5, 4096));

    size_t held = 0;
    size_t runs = 0;
    for (size_t i = 3; i < output.size() / 2; i++) {
        if (std::abs(output[i * 2] - output[(i - 1) * 2]) < 1e-12) {
            held++;
        } else {
            runs++;
        }
    }
    // Three repeats for every new sample, which is what a quarter rate means.
    QVERIFY2(runs > 0, "nothing was held at all");
    const double repeatsPerRun = static_cast<double>(held) / static_cast<double>(runs);
    QVERIFY2(std::abs(repeatsPerRun - 3.0) < 0.2, qPrintable(QString::number(repeatsPerRun)));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BitCrusherTest)
