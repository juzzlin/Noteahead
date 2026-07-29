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

#include "multi_engine_test.hpp"

#include "../../domain/dsp/multi_engine.hpp"

#include <QTest>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;
constexpr size_t warmUpCount = 1000;
constexpr size_t analysisCount = 48000;

MultiEngine createEngine(MultiEngine::Type type, float shape, float keyTrack, uint8_t note)
{
    MultiEngine engine;
    engine.setSampleRate(sampleRate);
    engine.setType(type);
    engine.setShape(shape);
    engine.setKeyTrack(keyTrack);
    engine.setNote(note);
    engine.reset();
    return engine;
}

// Renders analysisCount samples after letting the filter settle.
std::vector<float> render(MultiEngine & engine)
{
    for (size_t i = 0; i < warmUpCount; i++) {
        engine.nextSample();
    }
    std::vector<float> samples;
    samples.reserve(analysisCount);
    for (size_t i = 0; i < analysisCount; i++) {
        samples.push_back(engine.nextSample());
    }
    return samples;
}

std::vector<float> render(MultiEngine::Type type, float shape, float keyTrack, uint8_t note)
{
    auto engine = createEngine(type, shape, keyTrack, note);
    return render(engine);
}

double rms(const std::vector<float> & samples)
{
    double sum = 0;
    for (auto && sample : samples) {
        sum += static_cast<double>(sample) * static_cast<double>(sample);
    }
    return std::sqrt(sum / static_cast<double>(samples.size()));
}

// Zero-crossing rate tracks the dominant frequency of noise shaped by a band-pass.
double zeroCrossingRate(const std::vector<float> & samples)
{
    size_t crossings = 0;
    for (size_t i = 1; i < samples.size(); i++) {
        if ((samples[i - 1] < 0.0f) != (samples[i] < 0.0f)) {
            crossings++;
        }
    }
    return static_cast<double>(crossings) / static_cast<double>(samples.size());
}

} // namespace

void MultiEngineTest::test_lowType_keyTrackZero_shouldIgnoreNote()
{
    const auto low = render(MultiEngine::Type::Low, 0.3f, 0.0f, 36);
    const auto high = render(MultiEngine::Type::Low, 0.3f, 0.0f, 96);
    QCOMPARE(low, high);
}

void MultiEngineTest::test_highType_keyTrackZero_shouldIgnoreNote()
{
    const auto low = render(MultiEngine::Type::High, 0.8f, 0.0f, 36);
    const auto high = render(MultiEngine::Type::High, 0.8f, 0.0f, 96);
    QCOMPARE(low, high);
}

void MultiEngineTest::test_peakType_keyTrackZero_shouldIgnoreNote()
{
    const auto low = render(MultiEngine::Type::Peak, 0.2f, 0.0f, 36);
    const auto high = render(MultiEngine::Type::Peak, 0.2f, 0.0f, 96);
    QCOMPARE(low, high);
}

void MultiEngineTest::test_lowType_keyTrack_shouldOpenCutoffOnHigherNote()
{
    // A higher note lifts the low-pass cutoff, so more of the noise gets through.
    const auto atRoot = render(MultiEngine::Type::Low, 0.3f, 1.0f, 60);
    const auto threeOctavesUp = render(MultiEngine::Type::Low, 0.3f, 1.0f, 96);
    QVERIFY(rms(threeOctavesUp) > rms(atRoot) * 1.5);
}

void MultiEngineTest::test_highType_keyTrack_shouldCloseCutoffOnHigherNote()
{
    // A higher note lifts the high-pass cutoff, so less of the noise gets through.
    const auto atRoot = render(MultiEngine::Type::High, 0.8f, 1.0f, 60);
    const auto threeOctavesUp = render(MultiEngine::Type::High, 0.8f, 1.0f, 96);
    QVERIFY(rms(threeOctavesUp) < rms(atRoot) * 0.7);
}

void MultiEngineTest::test_peakType_keyTrack_shouldShiftBandOnHigherNote()
{
    const auto atRoot = render(MultiEngine::Type::Peak, 0.2f, 1.0f, 60);
    const auto threeOctavesUp = render(MultiEngine::Type::Peak, 0.2f, 1.0f, 96);
    QVERIFY(zeroCrossingRate(threeOctavesUp) > zeroCrossingRate(atRoot) * 2.0);
}

void MultiEngineTest::test_decimType_keyTrack_shouldRaiseRateOnHigherNote()
{
    const auto atRoot = render(MultiEngine::Type::Decim, 0.0f, 1.0f, 60);
    const auto octaveUp = render(MultiEngine::Type::Decim, 0.0f, 1.0f, 72);
    QVERIFY(zeroCrossingRate(octaveUp) > zeroCrossingRate(atRoot) * 1.5);
}

void MultiEngineTest::test_peakType_keyTrack_shouldTrackOctaveExactly()
{
    // Shape maps 0..1 to 110..880 Hz, so shape 0.2 is 264 Hz. An octave up must land on 528 Hz.
    const auto trackedOctaveUp = render(MultiEngine::Type::Peak, 0.2f, 1.0f, 72);
    const auto tunedByShape = render(MultiEngine::Type::Peak, (528.0f - 110.0f) / 770.0f, 0.0f, 60);
    const double tracked = zeroCrossingRate(trackedOctaveUp);
    const double tuned = zeroCrossingRate(tunedByShape);
    QVERIFY(std::abs(tracked - tuned) < tuned * 0.03);
}

void MultiEngineTest::test_keyTrack_shouldNotExceedNyquist()
{
    // Shape 1.0 already sits at the cutoff ceiling, so tracking must clamp instead of alias.
    const auto atCeiling = render(MultiEngine::Type::Low, 1.0f, 0.0f, 60);
    const auto trackedPastCeiling = render(MultiEngine::Type::Low, 1.0f, 1.0f, 127);
    for (auto && sample : trackedPastCeiling) {
        QVERIFY(std::isfinite(sample));
    }
    QCOMPARE(trackedPastCeiling, atCeiling);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MultiEngineTest)
