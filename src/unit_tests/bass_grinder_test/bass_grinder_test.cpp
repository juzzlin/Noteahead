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

#include "bass_grinder_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/effects/bass_grinder.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <numeric>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

//! Whole cycles rendered per measurement, so every harmonic lands exactly on a DFT bin and no
//! windowing is needed.
constexpr int Periods = 64;

//! Travel of the Split and Mid Freq controls, mirrored from the effect so a test can ask for a corner
//! in Hz rather than in control units.
constexpr double MinSplitHz = 20.0;
constexpr double MaxSplitHz = 800.0;
constexpr double MinMidHz = 200.0;
constexpr double MaxMidHz = 3000.0;

void setParameter(BassGrinder & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

//! Control position that puts the split at the given corner.
float splitAt(double frequency)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(frequency, MinSplitHz, MaxSplitHz));
}

//! Control position that puts the mid bell at the given corner.
float midFreqAt(double frequency)
{
    return static_cast<float>(ParameterMapper::unmapLogFrequency(frequency, MinMidHz, MaxMidHz));
}

//! Renders exactly Periods cycles of a sine into a buffer, discarding a warm-up run first so the DC
//! blocker and the tone stack have settled before anything is measured.
//!
//! The buffer length is chosen so the fundamental completes whole cycles, which puts every harmonic
//! exactly on a bin, but deliberately *not* so that a period is a whole number of samples. With an
//! integer samples-per-period, a harmonic that folds back around Nyquist lands right on top of
//! another harmonic of the same tone, which would make aliasing impossible to tell apart from the
//! harmonics the clipper is supposed to generate.
std::vector<double> renderSine(BassGrinder & effect, double frequency, double amplitude)
{
    effect.setSampleRate(SampleRate);

    const auto total = static_cast<size_t>(std::round(static_cast<double>(Periods) * SampleRate / frequency));

    const auto render = [&](std::vector<double> * out) {
        for (size_t i = 0; i < total; i++) {
            const double phase = 2.0 * std::numbers::pi * static_cast<double>(Periods) * static_cast<double>(i) / static_cast<double>(total);
            double left = amplitude * std::sin(phase);
            double right = left;
            effect.process(left, right);
            if (out) {
                out->push_back(left);
            }
        }
    };

    render(nullptr);
    std::vector<double> samples;
    samples.reserve(total);
    render(&samples);
    return samples;
}

//! Magnitude of the given harmonic. The buffer holds exactly Periods cycles, so harmonic k sits on
//! bin k * Periods and a single Goertzel-style sum resolves it exactly.
double harmonicMagnitude(const std::vector<double> & samples, int harmonic)
{
    const auto n = static_cast<double>(samples.size());
    const double bin = static_cast<double>(harmonic * Periods);
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        const double angle = 2.0 * std::numbers::pi * bin * static_cast<double>(i) / n;
        re += samples[i] * std::cos(angle);
        im += samples[i] * std::sin(angle);
    }
    return 2.0 * std::hypot(re, im) / n;
}

//! Total harmonic distortion: everything above the fundamental, relative to it.
double distortionRatio(const std::vector<double> & samples)
{
    const double fundamental = harmonicMagnitude(samples, 1);
    double harmonics = 0.0;
    for (int harmonic = 2; harmonic <= 8; harmonic++) {
        const double magnitude = harmonicMagnitude(samples, harmonic);
        harmonics += magnitude * magnitude;
    }
    return std::sqrt(harmonics) / std::max(1e-12, fundamental);
}

double meanLevel(const std::vector<double> & samples)
{
    return std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
}

double peakLevel(const std::vector<double> & samples)
{
    double peak = 0.0;
    for (const double sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

//! Energy sitting on anything that is not a harmonic of the input. Harmonics above Nyquist do not
//! exist in a sampled signal: either they were filtered away before decimation, or they folded back
//! to some unrelated frequency, and it is that folded rubbish this measures.
double inharmonicEnergy(const std::vector<double> & samples)
{
    double total = 0.0;
    for (const double sample : samples) {
        total += sample * sample;
    }

    double harmonics = 0.0;
    for (int harmonic = 1; static_cast<double>(harmonic * Periods) < static_cast<double>(samples.size()) * 0.5; harmonic++) {
        const double magnitude = harmonicMagnitude(samples, harmonic);
        harmonics += magnitude * magnitude * 0.5 * static_cast<double>(samples.size());
    }
    return std::max(0.0, total - harmonics);
}

//! A grinder with the tone stack flat and the clipper wide open, which is the starting point for
//! every measurement that is about the distortion rather than the EQ.
void configureClean(BassGrinder & effect)
{
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), 0.0f);
}

} // namespace

void BassGrinderTest::test_mix_zero_shouldPassSignalThrough()
{
    BassGrinder effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);

    double left = 0.5;
    double right = -0.3;
    effect.process(left, right);

    // A fully dry mix must leave the signal untouched however hard the clipper is driven.
    QVERIFY(qFuzzyCompare(left, 0.5));
    QVERIFY(qFuzzyCompare(right, -0.3));
}

void BassGrinderTest::test_blend_zero_shouldLeaveSignalClean()
{
    BassGrinder effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), 0.0f);

    const auto samples = renderSine(effect, 200.0, 0.8);

    // With none of the clipped band blended in, the two halves of the split sum back to the input and
    // the drive control is inaudible.
    QVERIFY(distortionRatio(samples) < 0.001);
    QVERIFY(std::abs(harmonicMagnitude(samples, 1) - 0.8) < 0.01);
}

void BassGrinderTest::test_split_high_shouldLeaveLowBandUndistorted()
{
    BassGrinder effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), splitAt(300.0));

    // The whole point of the effect: a kick's fundamental sits below the split, bypasses the clipper
    // and comes out whole no matter how hard the band above it is being ground up. Drive is at
    // maximum here on purpose -- this held only at moderate settings until the clipper's output was
    // filtered at the split too.
    const auto samples = renderSine(effect, 50.0, 0.8);
    QVERIFY(distortionRatio(samples) < 0.02);
    QVERIFY(std::abs(harmonicMagnitude(samples, 1) - 0.8) < 0.05);
}

void BassGrinderTest::test_split_low_shouldDistortTheWholeBand()
{
    BassGrinder effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), 0.0f);

    // At the bottom of its travel the split is below the audio band, so the same tone that survived
    // the previous test now goes through the clipper intact.
    const auto samples = renderSine(effect, 50.0, 0.8);
    QVERIFY(distortionRatio(samples) > 0.3);
}

void BassGrinderTest::test_drive_higher_shouldSaturateMore()
{
    const auto saturationFor = [](float drive) {
        BassGrinder effect;
        setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), 0.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyDrive(), drive);
        renderSine(effect, 200.0, 0.5);
        return effect.saturationDb();
    };

    QVERIFY(saturationFor(0.8f) < saturationFor(0.2f));
    QVERIFY(saturationFor(0.2f) < 0.0f);
}

void BassGrinderTest::test_clipper_shouldGenerateEvenHarmonics()
{
    const auto evenHarmonicRatio = [](float drive) {
        BassGrinder effect;
        setParameter(effect, Constants::NahdXml::xmlKeyDrive(), drive);
        setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), 0.0f);
        const auto samples = renderSine(effect, 200.0, 0.5);
        return harmonicMagnitude(samples, 2) / harmonicMagnitude(samples, 1);
    };

    // Odd harmonics dominate, as they do in any clipper. What matters is that the even ones are
    // there and get stronger the harder it is pushed: mismatched diode thresholds alone would lose
    // them at the top of the control, where both halves saturate into a symmetric square, and it is
    // the standing bias that keeps the duty cycle off half and holds them up.
    QVERIFY(evenHarmonicRatio(1.0f) > 0.05);
    QVERIFY(evenHarmonicRatio(1.0f) > evenHarmonicRatio(0.25f));
}

void BassGrinderTest::test_output_shouldNotDriftToDc()
{
    BassGrinder effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), 0.0f);

    const auto samples = renderSine(effect, 200.0, 0.8);

    // The asymmetric curve puts an offset on its output; the DC blocker has to take it back off so
    // nothing downstream inherits it.
    QVERIFY(std::abs(meanLevel(samples)) < 0.001);
}

void BassGrinderTest::test_color_on_shouldScoopMids()
{
    const auto levelAt = [](float color, double frequency) {
        BassGrinder effect;
        configureClean(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyColor(), color);
        return harmonicMagnitude(renderSine(effect, frequency, 0.5), 1);
    };

    // The Color voicing is a scooped smile: mids down, weight and edge up.
    QVERIFY(levelAt(1.0f, 500.0) < levelAt(0.0f, 500.0) * 0.6);
    QVERIFY(levelAt(1.0f, 60.0) > levelAt(0.0f, 60.0) * 1.1);
    QVERIFY(levelAt(1.0f, 8000.0) > levelAt(0.0f, 8000.0) * 1.1);
}

void BassGrinderTest::test_bassGain_shouldBoostLowEnd()
{
    const auto levelFor = [](float gain) {
        BassGrinder effect;
        configureClean(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyBassGain(), gain);
        return harmonicMagnitude(renderSine(effect, 40.0, 0.5), 1);
    };

    QVERIFY(levelFor(1.0f) > levelFor(0.5f) * 4.0); // At least +12 dB of the shelf's +15
    QVERIFY(levelFor(0.0f) < levelFor(0.5f) * 0.25);
}

void BassGrinderTest::test_highGain_shouldBoostTopEnd()
{
    const auto levelFor = [](float gain) {
        BassGrinder effect;
        configureClean(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyHighGain(), gain);
        return harmonicMagnitude(renderSine(effect, 8000.0, 0.5), 1);
    };

    QVERIFY(levelFor(1.0f) > levelFor(0.5f) * 4.0);
    QVERIFY(levelFor(0.0f) < levelFor(0.5f) * 0.25);
}

void BassGrinderTest::test_midFreq_shouldMoveTheBell()
{
    const auto levelAt = [](float midFreq, double frequency) {
        BassGrinder effect;
        configureClean(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyMidGain(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyMidFreq(), midFreq);
        return harmonicMagnitude(renderSine(effect, frequency, 0.5), 1);
    };

    // Each end of the sweep boosts its own corner more than the other end does.
    QVERIFY(levelAt(midFreqAt(MinMidHz), MinMidHz) > levelAt(midFreqAt(MaxMidHz), MinMidHz));
    QVERIFY(levelAt(midFreqAt(MaxMidHz), MaxMidHz) > levelAt(midFreqAt(MinMidHz), MaxMidHz));
}

void BassGrinderTest::test_gain_shouldScaleOutput()
{
    const auto peakFor = [](float gain) {
        BassGrinder effect;
        configureClean(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyGain(), gain);
        return peakLevel(renderSine(effect, 200.0, 0.5));
    };

    // The control spans +/- 12 dB, so the top of its travel is about four times the middle.
    QVERIFY(std::abs(peakFor(1.0f) / peakFor(0.5f) - 3.98) < 0.1);
}

void BassGrinderTest::test_oversampling_shouldSuppressAliasing()
{
    const auto aliasEnergyFor = [](uint8_t factor) {
        BassGrinder effect;
        setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.9f);
        setParameter(effect, Constants::NahdXml::xmlKeyBlend(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeySplitFreq(), 0.0f);
        effect.setOversampleFactor(factor);
        // Not a whole number of samples per period, or every folded harmonic would land right on top
        // of a real one and aliasing would be impossible to measure.
        return inharmonicEnergy(renderSine(effect, 4100.0, 0.8));
    };

    // The clipper generates harmonics well past Nyquist; rendering it at a higher rate has to push
    // them into the decimation filter's stopband instead of folding them back into the band.
    QVERIFY(aliasEnergyFor(4) < aliasEnergyFor(1) * 0.5);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BassGrinderTest)
