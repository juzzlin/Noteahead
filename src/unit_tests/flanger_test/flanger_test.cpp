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

#include "flanger_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/effects/flanger.hpp"

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

void FlangerTest::test_mixZero_shouldPassThrough()
{
    Flanger effect;
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);
    const auto input = tone(440.0, 0.5, 4096);
    const auto output = render(effect, input);
    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY2(std::abs(output[i] - input[i]) < 1e-9, qPrintable(QString::number(i)));
    }
}

void FlangerTest::test_sweep_shouldCombTheSignal()
{
    // What a flanger does that a delay does not: the level moves, because the dry and the delayed
    // copy fall in and out of phase as the delay sweeps.
    Flanger effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
    const auto output = render(effect, tone(1000.0, 0.5, static_cast<size_t>(SampleRate)));

    // Peak per tenth of a second, so a sweep of about a second shows up as a spread between them.
    std::vector<double> peaks;
    const size_t window = static_cast<size_t>(SampleRate) / 10;
    for (size_t start = 0; start + window < output.size() / 2; start += window) {
        double value = 0.0;
        for (size_t i = start; i < start + window; i++) {
            value = std::max(value, std::abs(output[i * 2]));
        }
        peaks.push_back(value);
    }
    const auto [lowest, highest] = std::minmax_element(peaks.begin(), peaks.end());
    QVERIFY2(*highest > *lowest * 1.2, qPrintable(QString::number(*lowest) + " to " + QString::number(*highest)));
}

void FlangerTest::test_feedback_shouldBeNeutralAtHalfTravel()
{
    // The control is centred: half travel is no feedback, and either side is one of the two
    // polarities. A flanger whose feedback could only be positive would be half an effect.
    Flanger effect;
    setParameter(effect, Constants::NahdXml::xmlKeyFeedback(), 0.5f);
    const auto neutral = render(effect, tone(1000.0, 0.5, 8192));

    Flanger positive;
    setParameter(positive, Constants::NahdXml::xmlKeyFeedback(), 1.0f);
    const auto resonant = render(positive, tone(1000.0, 0.5, 8192));

    QVERIFY2(peak(resonant) > peak(neutral), qPrintable(QString::number(peak(neutral)) + " vs " + QString::number(peak(resonant))));
}

void FlangerTest::test_rateDivider_shouldSlowTheSweep()
{
    const auto movement = [](float divider) {
        Flanger effect;
        setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyRateDivider(), divider);
        const auto output = render(effect, tone(1000.0, 0.5, static_cast<size_t>(SampleRate) / 4));
        // How far the level travels over a quarter second. A slower sweep covers less of its cycle
        // in the same time, so it moves less.
        double lowest = 1.0;
        double highest = 0.0;
        const size_t window = 2048;
        for (size_t start = 0; start + window < output.size() / 2; start += window) {
            double value = 0.0;
            for (size_t i = start; i < start + window; i++) {
                value = std::max(value, std::abs(output[i * 2]));
            }
            lowest = std::min(lowest, value);
            highest = std::max(highest, value);
        }
        return highest - lowest;
    };

    QVERIFY2(movement(1.0f) > movement(32.0f) * 1.5, "the divider did not slow the sweep");
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FlangerTest)
