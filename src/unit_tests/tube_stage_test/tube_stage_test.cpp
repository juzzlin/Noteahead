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

#include "tube_stage_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/tube_stage.hpp"

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

void setParameter(TubeStage & effect, const QString & key, float value)
{
    // Not a silent no-op on an unknown key: a parameter that has been renamed would otherwise
    // leave every test that sets it asserting against the default and still passing.
    const auto p = effect.parameter(key.toStdString());
    Q_ASSERT(p.has_value());
    p->get().update(value);
    effect.sync();
}

//! Renders exactly Periods cycles of a sine into a buffer, discarding a warm-up run first so the DC
//! blocker and the tilt filter have settled before anything is measured.
//!
//! The buffer length is chosen so the fundamental completes whole cycles, which puts every harmonic
//! exactly on a bin, but deliberately *not* so that a period is a whole number of samples. With an
//! integer samples-per-period, a harmonic that folds back around Nyquist lands right on top of
//! another harmonic of the same tone, which would make aliasing impossible to tell apart from the
//! harmonics the valve is supposed to generate.
std::vector<double> renderSine(TubeStage & effect, double frequency, double amplitude)
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

} // namespace

void TubeStageTest::test_mixZero_shouldPassSignalThrough()
{
    TubeStage effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.9f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);

    double left = 0.5;
    double right = -0.3;
    effect.process(left, right);

    // A fully dry mix must leave the signal untouched however hard the valve is driven.
    QVERIFY(qFuzzyCompare(left, 0.5));
    QVERIFY(qFuzzyCompare(right, -0.3));
}

void TubeStageTest::test_triode_shouldGenerateEvenHarmonics()
{
    TubeStage effect;
    setParameter(effect, Constants::NahdXml::xmlKeyMode(), 0.0f); // Triode
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.5f);

    const auto samples = renderSine(effect, 500.0, 0.5);
    const double second = harmonicMagnitude(samples, 2);
    const double fundamental = harmonicMagnitude(samples, 1);

    // The asymmetric triode curve is what produces second-harmonic warmth in the first place, so it
    // has to be measurably there rather than merely claimed in a comment.
    QVERIFY2(second > fundamental * 0.01,
             QString("Second harmonic was %1 against a fundamental of %2").arg(second).arg(fundamental).toUtf8().constData());
}

void TubeStageTest::test_pentode_centredBias_shouldFavourOddHarmonics()
{
    TubeStage effect;
    setParameter(effect, Constants::NahdXml::xmlKeyMode(), 1.0f); // Pentode
    setParameter(effect, Constants::NahdXml::xmlKeyBias(), 0.5f);
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.5f);

    const auto samples = renderSine(effect, 500.0, 0.5);
    const double second = harmonicMagnitude(samples, 2);
    const double third = harmonicMagnitude(samples, 3);

    // A symmetric curve sitting at its centre point cannot generate even harmonics, so what it does
    // generate has to be odd. This is what makes Pentode a different colour rather than a relabel.
    QVERIFY2(second < third * 0.1,
             QString("Second harmonic %1 was not clearly below the third %2").arg(second).arg(third).toUtf8().constData());
}

void TubeStageTest::test_bias_offCentre_shouldIncreaseEvenHarmonics()
{
    const auto secondHarmonicAtBias = [](float bias) {
        TubeStage effect;
        setParameter(effect, Constants::NahdXml::xmlKeyMode(), 1.0f); // Pentode, symmetric at centre
        setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.5f);
        setParameter(effect, Constants::NahdXml::xmlKeyBias(), bias);
        return harmonicMagnitude(renderSine(effect, 500.0, 0.5), 2);
    };

    const double centred = secondHarmonicAtBias(0.5f);
    const double offCentre = secondHarmonicAtBias(0.9f);

    // Bias is the operating point: moving it off centre is exactly what tips a symmetric curve into
    // generating even harmonics.
    QVERIFY2(offCentre > centred * 5.0,
             QString("Off-centre bias gave %1 against %2 when centred").arg(offCentre).arg(centred).toUtf8().constData());
}

void TubeStageTest::test_output_shouldNotDriftToDc()
{
    TubeStage effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.8f);
    setParameter(effect, Constants::NahdXml::xmlKeyBias(), 0.95f);

    const auto samples = renderSine(effect, 500.0, 0.7);

    // Hard against the bias extreme the curve is very lopsided, which would leave a standing offset
    // on the output for the rest of the rack to deal with if it were not blocked.
    QVERIFY2(std::abs(meanLevel(samples)) < 0.01,
             QString("Output sat at a DC offset of %1").arg(meanLevel(samples)).toUtf8().constData());
}

void TubeStageTest::test_tone_shouldTiltTheSpectrum()
{
    const auto gainAt = [](double frequency, float tone) {
        TubeStage effect;
        setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyTone(), tone);
        // Small signal, so the valve stays near linear and only the tilt is being measured.
        return harmonicMagnitude(renderSine(effect, frequency, 0.05), 1) / 0.05;
    };

    const double darkLow = gainAt(100.0, 0.0f);
    const double darkHigh = gainAt(5000.0, 0.0f);
    const double brightLow = gainAt(100.0, 1.0f);
    const double brightHigh = gainAt(5000.0, 1.0f);

    // A tilt pivots: turning it up has to lift the top and drop the bottom, not just add level.
    QVERIFY2(darkLow > darkHigh,
             QString("Dark tone did not favour lows: %1 vs %2").arg(darkLow).arg(darkHigh).toUtf8().constData());
    QVERIFY2(brightHigh > brightLow,
             QString("Bright tone did not favour highs: %1 vs %2").arg(brightHigh).arg(brightLow).toUtf8().constData());
}

void TubeStageTest::test_drive_higher_shouldSaturateMore()
{
    const auto saturationAtDrive = [](float drive) {
        TubeStage effect;
        setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), drive);
        renderSine(effect, 500.0, 0.5);
        return effect.saturationDb();
    };

    const float gentle = saturationAtDrive(0.1f);
    const float hard = saturationAtDrive(0.9f);

    // The meter reports how hard the valve is working, so it has to read further down as the drive
    // goes up.
    QVERIFY2(hard < gentle,
             QString("Hard drive read %1 dB against %2 dB when gentle").arg(hard).arg(gentle).toUtf8().constData());
    QVERIFY(hard < 0.0f);
}

void TubeStageTest::test_gain_shouldScaleOutput()
{
    const auto peakAtGain = [](float gain) {
        TubeStage effect;
        setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.3f);
        setParameter(effect, Constants::NahdXml::xmlKeyGain(), gain);
        return peakLevel(renderSine(effect, 500.0, 0.4));
    };

    QVERIFY(peakAtGain(1.0f) > peakAtGain(0.5f));
    QVERIFY(peakAtGain(0.5f) > peakAtGain(0.0f));
}

void TubeStageTest::test_oversampling_shouldSuppressAliasing()
{
    const auto aliasingAtFactor = [](uint8_t factor) {
        TubeStage effect;
        effect.setOversampleFactor(factor);
        setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 1.0f);
        // High enough that the harmonics a hard-driven valve generates run past Nyquist.
        return inharmonicEnergy(renderSine(effect, 4100.0, 0.8));
    };

    const double atOne = aliasingAtFactor(1);
    const double atFour = aliasingAtFactor(4);

    // Without oversampling those harmonics fold back as inharmonic rubbish; the whole point of
    // shaping at a higher rate is that the decimation filter removes them instead.
    QVERIFY2(atFour < atOne * 0.5,
             QString("Oversampled aliasing %1 was not clearly below %2 at 1x").arg(atFour).arg(atOne).toUtf8().constData());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::TubeStageTest)
