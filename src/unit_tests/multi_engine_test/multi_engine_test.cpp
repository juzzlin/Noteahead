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

MultiEngine createVpm(MultiEngine::VpmType type, float shape, double frequency, const MultiEngine::VpmOffsets & offsets = {})
{
    MultiEngine engine;
    engine.setSampleRate(sampleRate);
    engine.setEngine(MultiEngine::Engine::Vpm);
    engine.setVpmType(type);
    engine.setShape(shape);
    engine.setVpmOffsets(offsets);
    engine.setFrequency(frequency);
    engine.reset();
    engine.trigger(true);
    return engine;
}

std::vector<float> renderSamples(MultiEngine & engine, size_t count)
{
    std::vector<float> samples;
    samples.reserve(count);
    for (size_t i = 0; i < count; i++) {
        samples.push_back(engine.nextSample());
    }
    return samples;
}

// Amplitude of the component at the given frequency. Exact when the window holds whole cycles of
// it, which every frequency the tests ask about does.
double amplitudeAt(const std::vector<float> & samples, double frequency)
{
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        const double angle = 2.0 * M_PI * frequency * static_cast<double>(i) / sampleRate;
        re += static_cast<double>(samples[i]) * std::cos(angle);
        im -= static_cast<double>(samples[i]) * std::sin(angle);
    }
    return 2.0 * std::hypot(re, im) / static_cast<double>(samples.size());
}

// Share of the power that sits in the fundamental, 1.0 for a pure sine.
double fundamentalShare(const std::vector<float> & samples, double frequency)
{
    const double fundamental = amplitudeAt(samples, frequency);
    const double total = rms(samples);
    return fundamental * fundamental / 2.0 / (total * total);
}

constexpr double vpmFrequency = 200.0;

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

void MultiEngineTest::test_vpm_sin1_shapeZero_shouldBePureSine()
{
    auto engine = createVpm(MultiEngine::VpmType::Sin1, 0.0f, vpmFrequency);
    const auto samples = renderSamples(engine, analysisCount);
    QVERIFY(std::abs(amplitudeAt(samples, vpmFrequency) - 1.0) < 0.001);
    QVERIFY(fundamentalShare(samples, vpmFrequency) > 0.9999);
}

void MultiEngineTest::test_vpm_sin1_shape_shouldPlayFrequency()
{
    auto engine = createVpm(MultiEngine::VpmType::Sin1, 0.3f, vpmFrequency);
    const auto samples = renderSamples(engine, analysisCount);
    // A 1:1 pair puts everything on harmonics of the note, nothing in between.
    QVERIFY(amplitudeAt(samples, vpmFrequency) > 0.1);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 2.0) > 0.1);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 1.5) < 0.001);
}

void MultiEngineTest::test_vpm_sin3_sidebands_shouldSkipThirdHarmonic()
{
    auto engine = createVpm(MultiEngine::VpmType::Sin3, 0.3f, vpmFrequency);
    const auto samples = renderSamples(engine, analysisCount);
    // Sidebands at f +- 3kf: f, 2f, 4f, 5f, 7f... never a multiple of three.
    QVERIFY(amplitudeAt(samples, vpmFrequency * 2.0) > 0.1);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 4.0) > 0.1);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 3.0) < 0.001);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 6.0) < 0.001);
}

void MultiEngineTest::test_vpm_squ2_shapeZero_shouldHaveOnlyOddHarmonics()
{
    auto engine = createVpm(MultiEngine::VpmType::Squ2, 0.0f, vpmFrequency);
    const auto samples = renderSamples(engine, analysisCount);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 3.0) > 0.1);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 2.0) < 0.001);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 4.0) < 0.001);
}

void MultiEngineTest::test_vpm_saw1_shapeZero_shouldFallAsOneOverN()
{
    auto engine = createVpm(MultiEngine::VpmType::Saw1, 0.0f, vpmFrequency);
    const auto samples = renderSamples(engine, analysisCount);
    const double first = amplitudeAt(samples, vpmFrequency);
    QVERIFY(std::abs(amplitudeAt(samples, vpmFrequency * 2.0) / first - 0.5) < 0.01);
    QVERIFY(std::abs(amplitudeAt(samples, vpmFrequency * 3.0) / first - 1.0 / 3.0) < 0.01);
    // Band limited: nothing folds back between the harmonics.
    QVERIFY(amplitudeAt(samples, vpmFrequency * 1.5) < 0.001);
}

void MultiEngineTest::test_vpm_ratioOffset_twoToOne_shouldHaveOnlyOddHarmonics()
{
    auto engine = createVpm(MultiEngine::VpmType::Sin1, 0.3f, vpmFrequency);
    engine.setVpmRatioOffset(MultiEngine::VpmRatioOffsetUnity + 1);
    const auto samples = renderSamples(engine, analysisCount);
    QCOMPARE(MultiEngine::vpmRatioOffsetNames().at(MultiEngine::VpmRatioOffsetUnity + 1), std::string { "2:1" });
    QVERIFY(amplitudeAt(samples, vpmFrequency * 3.0) > 0.1);
    QVERIFY(amplitudeAt(samples, vpmFrequency * 2.0) < 0.001);
}

void MultiEngineTest::test_vpm_decay2_envelope_shouldSettleIntoSine()
{
    auto engine = createVpm(MultiEngine::VpmType::Decay2, 0.0f, vpmFrequency);
    const size_t window = 4800; // 0.1 s, twenty whole cycles
    const auto onset = renderSamples(engine, window);
    renderSamples(engine, static_cast<size_t>(sampleRate * 3.0) - window);
    const auto settled = renderSamples(engine, window);
    QVERIFY(fundamentalShare(onset, vpmFrequency) < 0.5);
    QVERIFY(fundamentalShare(settled, vpmFrequency) > 0.9999);
}

void MultiEngineTest::test_vpm_decay2_shapeModIntMinusFull_shouldDisableEnvelope()
{
    auto engine = createVpm(MultiEngine::VpmType::Decay2, 0.0f, vpmFrequency, { .shapeModInt = -1.0 });
    const auto onset = renderSamples(engine, 4800);
    QVERIFY(fundamentalShare(onset, vpmFrequency) > 0.9999);
}

void MultiEngineTest::test_vpm_keyTrack_shouldReduceDepthOnHigherNote()
{
    // Depth shows as the share the fundamental loses to the sidebands.
    auto low = createVpm(MultiEngine::VpmType::Sin1, 0.3f, 200.0, { .modKeyTrack = 1.0 });
    auto high = createVpm(MultiEngine::VpmType::Sin1, 0.3f, 1600.0, { .modKeyTrack = 1.0 });
    const double lowShare = fundamentalShare(renderSamples(low, analysisCount), 200.0);
    const double highShare = fundamentalShare(renderSamples(high, analysisCount), 1600.0);
    QVERIFY(highShare > lowShare + 0.2);
}

void MultiEngineTest::test_vpm_allTypes_extremes_shouldStayBounded()
{
    const MultiEngine::VpmOffsets full { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
    for (size_t type = 0; type < MultiEngine::VpmTypeCount; type++) {
        for (const double frequency : { 20.0, 261.63, 4186.0, 20000.0 }) {
            auto engine = createVpm(static_cast<MultiEngine::VpmType>(type), 1.0f, frequency, full);
            engine.setVpmRatioOffset(MultiEngine::vpmRatioOffsets().size() - 1);
            for (auto && sample : renderSamples(engine, 24000)) {
                QVERIFY(std::isfinite(sample));
                QVERIFY(std::abs(sample) <= 1.0001f);
            }
        }
    }
}

void MultiEngineTest::test_vpm_reset_shouldReproduceRender()
{
    for (const auto type : { MultiEngine::VpmType::Air2, MultiEngine::VpmType::Creep }) {
        auto engine = createVpm(type, 0.5f, vpmFrequency);
        const auto first = renderSamples(engine, 4800);
        engine.reset();
        engine.trigger(true);
        QCOMPARE(renderSamples(engine, 4800), first);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MultiEngineTest)
