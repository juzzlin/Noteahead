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

#include "tremolo_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/tremolo.hpp"

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

//! Peak over the last quarter of a buffer.
//!
//! A gate starts open and takes its release to close, so the loudest sample in a gated buffer is
//! always one of the first: measured over the whole thing, every one of these tests would be
//! reading the gate's release time rather than what it settles at.
double settledPeak(const std::vector<double> & buffer)
{
    const auto from = buffer.size() / 4 * 3;
    double value = 0.0;
    for (size_t i = from; i < buffer.size(); i++) {
        value = std::max(value, std::abs(buffer[i]));
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

void TremoloTest::test_depthZero_shouldPassThrough()
{
    Tremolo effect;
    setParameter(effect, Constants::NahdXml::xmlKeyIntensity(), 0.0f);
    const auto input = tone(440.0, 0.5, 4096);
    const auto output = render(effect, input);
    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY2(std::abs(output[i] - input[i]) < 1e-9, qPrintable(QString::number(i)));
    }
}

void TremoloTest::test_depth_shouldOnlyEverTakeLevelAway()
{
    // Modulating either side of unity would make turning the depth up turn the track up, which is
    // heard as the effect being louder rather than deeper.
    Tremolo effect;
    setParameter(effect, Constants::NahdXml::xmlKeyIntensity(), 1.0f);
    const auto input = tone(440.0, 0.5, static_cast<size_t>(SampleRate));
    const auto output = render(effect, input);
    QVERIFY2(peak(output) <= peak(input) + 1e-9, qPrintable(QString::number(peak(output))));
}

void TremoloTest::test_fullDepth_shouldReachSilence()
{
    Tremolo effect;
    setParameter(effect, Constants::NahdXml::xmlKeyIntensity(), 1.0f);
    const auto output = render(effect, tone(2000.0, 0.5, static_cast<size_t>(SampleRate)));

    double quietest = 1.0;
    const size_t window = 512;
    for (size_t start = 0; start + window < output.size() / 2; start += window) {
        double value = 0.0;
        for (size_t i = start; i < start + window; i++) {
            value = std::max(value, std::abs(output[i * 2]));
        }
        quietest = std::min(quietest, value);
    }
    QVERIFY2(quietest < 0.02, qPrintable(QString::number(quietest)));
}

void TremoloTest::test_stereoPhase_shouldDuckTheChannelsApart()
{
    Tremolo effect;
    setParameter(effect, Constants::NahdXml::xmlKeyIntensity(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyStereoPhase(), 1.0f);
    const auto output = render(effect, tone(2000.0, 0.5, static_cast<size_t>(SampleRate)));

    // In opposition the two channels are never quiet at the same moment, so the difference between
    // them is what the effect is. In step they would be identical.
    double largestDifference = 0.0;
    for (size_t i = 0; i < output.size() / 2; i++) {
        largestDifference = std::max(largestDifference, std::abs(std::abs(output[i * 2]) - std::abs(output[i * 2 + 1])));
    }
    QVERIFY2(largestDifference > 0.1, qPrintable(QString::number(largestDifference)));
}


} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TremoloTest)
