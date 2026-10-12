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

#include "fm_drum_engine_test.hpp"

#include "../../domain/dsp/drum/fm_drum_engine.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

//! Serial, 4>3>2>1, which is also what the engine starts out with.
const std::array<uint8_t, FmDrumEngine::OperatorCount> serialModulators { 0b0010, 0b0100, 0b1000, 0b0000 };
const std::array<uint8_t, FmDrumEngine::OperatorCount> noModulators { 0b0000, 0b0000, 0b0000, 0b0000 };

void prepare(FmDrumEngine & engine, double sampleRate = SampleRate, uint8_t oversampleFactor = 1)
{
    engine.setSampleRate(sampleRate);
    engine.setOversampleFactor(oversampleFactor);
    engine.setBaseFrequency(100.0);
    engine.setOperatorDecay(0, 2.0);
}

std::vector<double> render(FmDrumEngine & engine, size_t frames)
{
    std::vector<double> samples;
    samples.reserve(frames);
    for (size_t i = 0; i < frames; i++) {
        samples.push_back(engine.nextSample());
    }
    return samples;
}

size_t frames(double seconds, double sampleRate = SampleRate)
{
    return static_cast<size_t>(seconds * sampleRate);
}

//! Frequency from the rising zero crossings in [from, to), which is all a single tone needs.
double frequency(const std::vector<double> & samples, size_t from, size_t to)
{
    std::vector<double> crossings;
    for (size_t i = std::max<size_t>(from, 1); i < to && i < samples.size(); i++) {
        if (samples.at(i - 1) < 0.0 && samples.at(i) >= 0.0) {
            const double fraction = samples.at(i - 1) / (samples.at(i - 1) - samples.at(i));
            crossings.push_back(static_cast<double>(i - 1) + fraction);
        }
    }
    if (crossings.size() < 2) {
        return 0.0;
    }
    return SampleRate * static_cast<double>(crossings.size() - 1) / (crossings.back() - crossings.front());
}

double rms(const std::vector<double> & samples)
{
    double sum = 0.0;
    for (const double sample : samples) {
        sum += sample * sample;
    }
    return samples.empty() ? 0.0 : std::sqrt(sum / static_cast<double>(samples.size()));
}

double peak(const std::vector<double> & samples)
{
    double result = 0.0;
    for (const double sample : samples) {
        result = std::max(result, std::abs(sample));
    }
    return result;
}

//! Largest step between neighbouring samples, which is what a click is.
double largestStep(const std::vector<double> & samples)
{
    double result = 0.0;
    for (size_t i = 1; i < samples.size(); i++) {
        result = std::max(result, std::abs(samples.at(i) - samples.at(i - 1)));
    }
    return result;
}

//! How much of the signal is in its changes rather than its level: rises with the sidebands.
double brightness(const std::vector<double> & samples)
{
    std::vector<double> differences;
    for (size_t i = 1; i < samples.size(); i++) {
        differences.push_back(samples.at(i) - samples.at(i - 1));
    }
    return rms(differences) / std::max(1.0e-12, rms(samples));
}

} // namespace

void FmDrumEngineTest::test_trigger_defaultVoice_shouldPlayTheBaseFrequency()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.trigger(1.0f);

    const auto samples = render(engine, frames(0.2));

    QVERIFY(engine.isActive());
    QVERIFY(std::abs(frequency(samples, 0, samples.size()) - 100.0) < 0.5);
    QVERIFY(peak(samples) > 0.3);
    QVERIFY(peak(samples) <= 0.5 + 1.0e-9);
}

void FmDrumEngineTest::test_trigger_twice_shouldRenderTheSameHit()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.setOperatorDecay(0, 0.1);
    engine.setAlgorithm(serialModulators, 0b0001);
    engine.setOperatorLevel(1, 0.6);
    engine.setOperatorLevel(3, 0.4);
    engine.setOperatorRatio(1, 1.41);
    engine.setFeedback(0.8);
    engine.setNoiseLevel(0.5);
    engine.setNoiseModulation(0.3);
    engine.setNoiseFilterMode(FmDrumEngine::NoiseFilterMode::BandPass);
    engine.setNoiseCutoff(0.7);
    engine.setPitchEnvelopeAmount(12.0);

    engine.trigger(0.8f);
    const auto first = render(engine, frames(1.0));
    QVERIFY(!engine.isActive());

    engine.trigger(0.8f);
    const auto second = render(engine, frames(1.0));

    QCOMPARE(second, first);
}

void FmDrumEngineTest::test_trigger_whileSounding_shouldNotStep()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.trigger(1.0f);
    // Part way through a cycle, where restarting the phase is a jump of about half the peak
    auto samples = render(engine, frames(0.0123));
    QVERIFY(std::abs(samples.back()) > 0.2);

    engine.trigger(1.0f);
    const auto after = render(engine, frames(0.01));
    samples.insert(samples.end(), after.begin(), after.end());

    QVERIFY2(largestStep(samples) < 0.03, qPrintable(QString { "step %1" }.arg(largestStep(samples))));
}

void FmDrumEngineTest::test_pitchEnvelope_positiveAmount_shouldFallBackToTheBase()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.setPitchEnvelopeAmount(24.0);
    engine.setPitchEnvelopeDecay(0.02);
    engine.trigger(1.0f);

    const auto samples = render(engine, frames(0.5));

    const double start = frequency(samples, 0, frames(0.01));
    const double settled = frequency(samples, frames(0.3), frames(0.5));
    QVERIFY2(start > 250.0, qPrintable(QString { "started at %1 Hz" }.arg(start)));
    QVERIFY2(std::abs(settled - 100.0) < 0.5, qPrintable(QString { "settled at %1 Hz" }.arg(settled)));
}

void FmDrumEngineTest::test_pitchEnvelope_negativeAmount_shouldStartBelowTheBase()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.setBaseFrequency(400.0);
    engine.setPitchEnvelopeAmount(-12.0);
    engine.setPitchEnvelopeDecay(0.05);
    engine.trigger(1.0f);

    const auto samples = render(engine, frames(0.5));

    QVERIFY(frequency(samples, 0, frames(0.01)) < 280.0);
    QVERIFY(std::abs(frequency(samples, frames(0.4), frames(0.5)) - 400.0) < 1.0);
}

void FmDrumEngineTest::test_isActive_afterTheDecay_shouldGoFalse()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.setOperatorDecay(0, 0.1);
    engine.trigger(1.0f);

    render(engine, frames(0.05));
    QVERIFY(engine.isActive());
    render(engine, frames(0.1));
    QVERIFY(!engine.isActive());
    QCOMPARE(engine.nextSample(), 0.0f);
}

void FmDrumEngineTest::test_isActive_silentCarriers_shouldGoFalseAtOnce()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.setOperatorLevel(0, 0.0);
    engine.trigger(1.0f);

    render(engine, 1);

    QVERIFY(!engine.isActive());
}

void FmDrumEngineTest::test_isActive_modulatorStillRinging_shouldNotKeepTheDrumAlive()
{
    // Nothing is heard of a modulator once its carrier has gone, however long it rings
    FmDrumEngine engine;
    prepare(engine);
    engine.setAlgorithm(serialModulators, 0b0001);
    engine.setOperatorDecay(0, 0.05);
    engine.setOperatorLevel(1, 1.0);
    engine.setOperatorDecay(1, 5.0);
    engine.trigger(1.0f);

    render(engine, frames(0.1));

    QVERIFY(!engine.isActive());
}

void FmDrumEngineTest::test_stop_shouldFadeOutWithoutAStep()
{
    FmDrumEngine engine;
    prepare(engine);
    engine.trigger(1.0f);
    auto samples = render(engine, frames(0.1));

    engine.stop();
    const auto fade = render(engine, frames(DrumEngine::ChokeFadeSeconds + 0.005));
    samples.insert(samples.end(), fade.begin(), fade.end());

    QVERIFY(!engine.isActive());
    QCOMPARE(fade.back(), 0.0);
    QVERIFY2(largestStep(samples) < 0.01, qPrintable(QString { "step %1" }.arg(largestStep(samples))));
}

void FmDrumEngineTest::test_algorithm_modulatedCarrier_shouldAddSidebands()
{
    FmDrumEngine plain;
    prepare(plain);
    plain.trigger(1.0f);
    const double plainBrightness = brightness(render(plain, frames(0.1)));

    FmDrumEngine modulated;
    prepare(modulated);
    modulated.setAlgorithm(serialModulators, 0b0001);
    modulated.setOperatorLevel(1, 0.5);
    modulated.setOperatorDecay(1, 2.0);
    modulated.trigger(1.0f);
    const auto samples = render(modulated, frames(0.1));

    QVERIFY(brightness(samples) > plainBrightness * 2.0);
}

void FmDrumEngineTest::test_noise_zeroAmounts_shouldLeaveTheToneUntouched()
{
    FmDrumEngine plain;
    prepare(plain);
    plain.trigger(1.0f);

    FmDrumEngine withNoiseOff;
    prepare(withNoiseOff);
    withNoiseOff.setNoiseFilterMode(FmDrumEngine::NoiseFilterMode::HighPass);
    withNoiseOff.setNoiseCutoff(0.3);
    withNoiseOff.setNoiseDecay(1.0);
    withNoiseOff.setNoiseLevel(0.0);
    withNoiseOff.setNoiseModulation(0.0);
    withNoiseOff.trigger(1.0f);

    QCOMPARE(render(withNoiseOff, frames(0.2)), render(plain, frames(0.2)));
}

void FmDrumEngineTest::test_noise_modulation_shouldRoughenTheTone()
{
    FmDrumEngine plain;
    prepare(plain);
    plain.trigger(1.0f);
    const auto plainSamples = render(plain, frames(0.05));

    FmDrumEngine rattled;
    prepare(rattled);
    rattled.setNoiseModulation(0.5);
    rattled.setNoiseTarget(0);
    rattled.setNoiseDecay(1.0);
    rattled.trigger(1.0f);
    const auto rattledSamples = render(rattled, frames(0.05));

    // Noise in the phase rather than in the output: no louder, only rougher. A shade of tolerance
    // because the noise can bring the first crest forward, to where the decay has taken off less.
    QVERIFY(brightness(rattledSamples) > brightness(plainSamples) * 3.0);
    QVERIFY(peak(rattledSamples) <= peak(plainSamples) * 1.01);
}

void FmDrumEngineTest::test_noise_oversampled_shouldKeepItsLevel()
{
    const auto noiseRms = [](uint8_t oversampleFactor) {
        FmDrumEngine engine;
        const double rate = SampleRate * oversampleFactor;
        prepare(engine, rate, oversampleFactor);
        engine.setOperatorLevel(0, 0.0);
        engine.setNoiseLevel(1.0);
        engine.setNoiseDecay(2.0);
        engine.setNoiseCurve(0.0);
        // Low passed well inside the base band, where every factor has the same noise to give
        engine.setNoiseCutoff(0.5);
        engine.trigger(1.0f);
        return rms(render(engine, frames(0.2, rate)));
    };

    const double base = noiseRms(1);
    QVERIFY(base > 0.01);
    for (const uint8_t factor : { 2, 4 }) {
        const double ratio = noiseRms(factor) / base;
        QVERIFY2(std::abs(20.0 * std::log10(ratio)) < 1.0, qPrintable(QString { "x%1 is %2 dB off" }.arg(factor).arg(20.0 * std::log10(ratio))));
    }
}

void FmDrumEngineTest::test_velocity_levelAmountZero_shouldIgnoreVelocity()
{
    FmDrumEngine soft;
    prepare(soft);
    soft.setVelocityToLevel(0.0);
    soft.trigger(0.2f);

    FmDrumEngine hard;
    prepare(hard);
    hard.setVelocityToLevel(0.0);
    hard.trigger(1.0f);

    QCOMPARE(render(soft, frames(0.1)), render(hard, frames(0.1)));
}

void FmDrumEngineTest::test_velocity_softHit_shouldBeQuieter()
{
    FmDrumEngine soft;
    prepare(soft);
    soft.trigger(0.25f);

    FmDrumEngine hard;
    prepare(hard);
    hard.trigger(1.0f);

    QVERIFY(std::abs(peak(render(soft, frames(0.1))) / peak(render(hard, frames(0.1))) - 0.25) < 0.01);
}

void FmDrumEngineTest::test_velocity_modulationAmount_softHit_shouldBeDuller()
{
    const auto hit = [](float velocity) {
        FmDrumEngine engine;
        prepare(engine);
        engine.setAlgorithm(serialModulators, 0b0001);
        engine.setOperatorLevel(1, 0.8);
        engine.setOperatorDecay(1, 2.0);
        engine.setVelocityToLevel(0.0);
        engine.setVelocityToModulation(1.0);
        engine.trigger(velocity);
        return brightness(render(engine, frames(0.1)));
    };

    QVERIFY(hit(0.2f) < hit(1.0f) * 0.7);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmDrumEngineTest)
