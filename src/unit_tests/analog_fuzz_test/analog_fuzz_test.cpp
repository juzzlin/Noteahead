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

#include "analog_fuzz_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/analog_fuzz.hpp"

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

void setParameter(AnalogFuzz & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

//! Puts the effect in a flat, quiet state, so that a test only has to set what it is about.
//! Everything here is off or out of the way: no drive to speak of, the corner above the band, no
//! resonance and the bias centred.
void setDefaults(AnalogFuzz & effect)
{
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyFuzz(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyBias(), 0.5f);
    setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyResonance(), 0.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyGain(), 0.5f);
}

//! Renders exactly Periods cycles of a sine, discarding a warm-up run first so the filter, which
//! rings for a while when resonant, and the DC blocker have both settled.
//!
//! The buffer length puts the fundamental on a whole number of cycles, so every harmonic lands
//! exactly on a bin, but deliberately not on a whole number of samples per period: with an integer
//! samples-per-period a harmonic that folds around Nyquist would land on top of another harmonic of
//! the same tone, and aliasing could not be told apart from the harmonics the stage is meant to make.
std::vector<double> renderSine(AnalogFuzz & effect, double frequency, double amplitude)
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

//! Everything above the fundamental, as a fraction of it.
double harmonicRatio(const std::vector<double> & samples)
{
    const double fundamental = harmonicMagnitude(samples, 1);
    double harmonics = 0.0;
    for (int harmonic = 2; static_cast<double>(harmonic * Periods) < static_cast<double>(samples.size()) * 0.5; harmonic++) {
        const double magnitude = harmonicMagnitude(samples, harmonic);
        harmonics += magnitude * magnitude;
    }
    return fundamental > 1.0e-12 ? std::sqrt(harmonics) / fundamental : 0.0;
}

double evenHarmonicEnergy(const std::vector<double> & samples)
{
    double energy = 0.0;
    for (int harmonic = 2; static_cast<double>(harmonic * Periods) < static_cast<double>(samples.size()) * 0.5; harmonic += 2) {
        const double magnitude = harmonicMagnitude(samples, harmonic);
        energy += magnitude * magnitude;
    }
    return energy;
}

double meanLevel(const std::vector<double> & samples)
{
    return std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
}

double rmsLevel(const std::vector<double> & samples)
{
    double total = 0.0;
    for (const double sample : samples) {
        total += sample * sample;
    }
    return std::sqrt(total / static_cast<double>(samples.size()));
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

void AnalogFuzzTest::test_mixZero_shouldPassSignalThrough()
{
    AnalogFuzz effect;
    setDefaults(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);

    double left = 0.5;
    double right = -0.3;
    effect.process(left, right);

    // A fully dry mix must leave the signal untouched however hard the stage is driven.
    QVERIFY(qFuzzyCompare(left, 0.5));
    QVERIFY(qFuzzyCompare(right, -0.3));
}

void AnalogFuzzTest::test_moderateDrive_shouldAudiblyDistort()
{
    // A device on a default patch puts out about -26 dBFS -- the rack leaves headroom for a whole
    // song -- and this is the level the effect has to work at. It did not: the drive stage's knee
    // was set at full scale, a signal that quiet never came near it, and the effect sat there
    // sounding like a slightly dull wire until the control was run right to the top.
    constexpr double deviceLevel = 0.048;

    AnalogFuzz effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.5f);

    const auto samples = renderSine(effect, 440.0, deviceLevel);

    QVERIFY(harmonicRatio(samples) > 0.15);

    // ...and it must not have gone quiet doing it: what is added has to be heard as drive rather
    // than as the signal dropping away.
    QVERIFY(rmsLevel(samples) > deviceLevel / std::numbers::sqrt2);
}

void AnalogFuzzTest::test_lowDrive_shouldPassTheLevelThrough()
{
    // The other end of the same calibration: putting the knee where the material is must not cost
    // anything at the quiet end. The curve is unity-slope through the origin, so the bottom of the
    // control has to hand back what it was given rather than a stage's worth of gain above it.
    AnalogFuzz effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.0f);

    const auto samples = renderSine(effect, 440.0, 0.048);

    QVERIFY(std::abs(rmsLevel(samples) / (0.048 / std::numbers::sqrt2) - 1.0) < 0.05);
}

void AnalogFuzzTest::test_drive_higher_shouldSaturateMore()
{
    AnalogFuzz effect;
    setDefaults(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.2f);
    renderSine(effect, 220.0, 0.5);
    const float gentle = effect.saturationDb();

    AnalogFuzz driven;
    setDefaults(driven);
    setParameter(driven, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    renderSine(driven, 220.0, 0.5);
    const float hard = driven.saturationDb();

    // The meter reads how much the stage is holding back, so more drive has to read further down.
    QVERIFY(hard < gentle);
}

void AnalogFuzzTest::test_drive_shouldNotRunAwayInLevel()
{
    // Drive is a character control. The shaper's ceiling and the filter's headroom between them are
    // what keep it from doubling as a volume knob, so sweeping it must not walk the output level
    // off. Without that the control is unusable: every move needs the output trimmed after it.
    // At the settings the effect ships with, which is where the control has to behave.
    const auto level = [](float drive) {
        AnalogFuzz effect;
        setParameter(effect, Constants::NahdXml::xmlKeyDrive(), drive);
        return rmsLevel(renderSine(effect, 220.0, 0.3));
    };

    const double quietLevel = level(0.0f);
    const double loudLevel = level(1.0f);

    QVERIFY(quietLevel > 1.0e-3);
    QVERIFY(loudLevel > 1.0e-3);

    // Within 6 dB across the whole sweep, either way. A signal this hot is already into the stage's
    // knee at the bottom of the control, so from there Drive can only compress it -- what must not
    // happen is the collapse this effect first came out with, where the level fell away as the
    // control came up and the drive read as the effect doing less rather than more.
    const double ratio = loudLevel / quietLevel;
    QVERIFY(ratio < 2.0);
    QVERIFY(ratio > 0.5);
}

void AnalogFuzzTest::test_fuzz_harderKnee_shouldGenerateMoreHarmonics()
{
    AnalogFuzz soft;
    setDefaults(soft);
    setParameter(soft, Constants::NahdXml::xmlKeyDrive(), 0.5f);
    setParameter(soft, Constants::NahdXml::xmlKeyFuzz(), 0.0f);
    const double softRatio = harmonicRatio(renderSine(soft, 220.0, 0.3));

    AnalogFuzz hard;
    setDefaults(hard);
    setParameter(hard, Constants::NahdXml::xmlKeyDrive(), 0.5f);
    setParameter(hard, Constants::NahdXml::xmlKeyFuzz(), 1.0f);
    const double hardRatio = harmonicRatio(renderSine(hard, 220.0, 0.3));

    QVERIFY(hardRatio > softRatio);
}

void AnalogFuzzTest::test_cutoff_shouldRemoveWhatIsAboveIt()
{
    AnalogFuzz open;
    setDefaults(open);
    const double openLevel = rmsLevel(renderSine(open, 4000.0, 0.3));

    AnalogFuzz closed;
    setDefaults(closed);
    // Corner right down at the bottom of its range, well below the tone.
    setParameter(closed, Constants::NahdXml::xmlKeyCutoff(), 0.0f);
    const double closedLevel = rmsLevel(renderSine(closed, 4000.0, 0.3));

    QVERIFY(closedLevel < openLevel * 0.1);
}

void AnalogFuzzTest::test_resonance_shouldLiftTheCorner()
{
    // Corner parked at 1 kHz by the log mapping the effect uses: 60 Hz to 12 kHz.
    const double position = std::log(1000.0 / 60.0) / std::log(12000.0 / 60.0);

    AnalogFuzz flat;
    setDefaults(flat);
    setParameter(flat, Constants::NahdXml::xmlKeyCutoff(), static_cast<float>(position));
    const double flatLevel = rmsLevel(renderSine(flat, 1000.0, 0.05));

    AnalogFuzz resonant;
    setDefaults(resonant);
    setParameter(resonant, Constants::NahdXml::xmlKeyCutoff(), static_cast<float>(position));
    setParameter(resonant, Constants::NahdXml::xmlKeyResonance(), 1.0f);
    const double resonantLevel = rmsLevel(renderSine(resonant, 1000.0, 0.05));

    QVERIFY(resonantLevel > flatLevel * 2.0);
}

void AnalogFuzzTest::test_resonance_underDrive_shouldGiveWay()
{
    // The reason this effect exists. The filter is inside the distortion rather than behind it, so
    // the resonant peak is bounded by the same ceiling the harmonics are: drive it hard and the peak
    // folds down instead of screaming. Chaining a distortion into a linear filter cannot do this --
    // there the peak keeps its height however hard the input is pushed.
    const double position = std::log(1000.0 / 60.0) / std::log(12000.0 / 60.0);

    const auto fundamental = [position](float drive, float resonance) {
        AnalogFuzz effect;
        setDefaults(effect);
        setParameter(effect, Constants::NahdXml::xmlKeyCutoff(), static_cast<float>(position));
        setParameter(effect, Constants::NahdXml::xmlKeyResonance(), resonance);
        setParameter(effect, Constants::NahdXml::xmlKeyDrive(), drive);
        // The fundamental only: the harmonics the stage makes are not what is being measured here.
        return harmonicMagnitude(renderSine(effect, 1000.0, 0.02), 1);
    };

    // How much the resonance lifts the corner, measured against the same drive setting with the
    // resonance flat. Taking it as a ratio is what keeps the drive stage's own gain out of the
    // measurement -- that gain is large, and it moves with Drive.
    const double gentleLift = fundamental(0.0f, 1.0f) / fundamental(0.0f, 0.0f);
    const double drivenLift = fundamental(1.0f, 1.0f) / fundamental(1.0f, 0.0f);

    QVERIFY(gentleLift > 2.0);
    QVERIFY(drivenLift < gentleLift * 0.5);
}

void AnalogFuzzTest::test_bias_offCentre_shouldIncreaseEvenHarmonics()
{
    AnalogFuzz centred;
    setDefaults(centred);
    setParameter(centred, Constants::NahdXml::xmlKeyDrive(), 0.5f);
    const double centredEven = evenHarmonicEnergy(renderSine(centred, 220.0, 0.3));

    AnalogFuzz biased;
    setDefaults(biased);
    setParameter(biased, Constants::NahdXml::xmlKeyDrive(), 0.5f);
    setParameter(biased, Constants::NahdXml::xmlKeyBias(), 0.85f);
    const double biasedEven = evenHarmonicEnergy(renderSine(biased, 220.0, 0.3));

    // A curve the signal sits symmetrically on can only make odd harmonics. Moving the operating
    // point off centre is what brings in the even ones.
    QVERIFY(biasedEven > centredEven * 2.0);
}

void AnalogFuzzTest::test_output_shouldNotDriftToDc()
{
    AnalogFuzz effect;
    setDefaults(effect);
    setParameter(effect, Constants::NahdXml::xmlKeyDrive(), 0.8f);
    setParameter(effect, Constants::NahdXml::xmlKeyBias(), 0.9f);

    const auto samples = renderSine(effect, 220.0, 0.4);

    // The asymmetric stage puts an offset on its output and the low-pass passes it straight through,
    // so without the DC blocker this is where it would show up.
    QVERIFY(std::abs(meanLevel(samples)) < 0.01 * rmsLevel(samples));
}

void AnalogFuzzTest::test_gain_shouldScaleOutput()
{
    AnalogFuzz unity;
    setDefaults(unity);
    setParameter(unity, Constants::NahdXml::xmlKeyDrive(), 0.4f);
    const double unityLevel = rmsLevel(renderSine(unity, 220.0, 0.3));

    AnalogFuzz trimmed;
    setDefaults(trimmed);
    setParameter(trimmed, Constants::NahdXml::xmlKeyDrive(), 0.4f);
    setParameter(trimmed, Constants::NahdXml::xmlKeyGain(), 0.0f); // -12 dB
    const double trimmedLevel = rmsLevel(renderSine(trimmed, 220.0, 0.3));

    QVERIFY(std::abs(trimmedLevel / unityLevel - 0.25) < 0.01);
}

void AnalogFuzzTest::test_oversampling_shouldSuppressAliasing()
{
    // A high tone driven hard makes harmonics far above Nyquist. Rendered at the base rate they fold
    // back into the band as inharmonic rubbish; rendered oversampled they are filtered away by the
    // decimator instead. The tone is deliberately not a submultiple of the sample rate: at 3 kHz it
    // would be exactly 16 samples per period and every folded harmonic would land on another
    // harmonic's bin, where this measurement counts it as signal.
    AnalogFuzz plain;
    setDefaults(plain);
    setParameter(plain, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    setParameter(plain, Constants::NahdXml::xmlKeyFuzz(), 1.0f);
    plain.setOversampleFactor(1);
    const double plainAliasing = inharmonicEnergy(renderSine(plain, 3100.0, 0.5));

    AnalogFuzz oversampled;
    setDefaults(oversampled);
    setParameter(oversampled, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    setParameter(oversampled, Constants::NahdXml::xmlKeyFuzz(), 1.0f);
    oversampled.setOversampleFactor(4);
    const double oversampledAliasing = inharmonicEnergy(renderSine(oversampled, 3100.0, 0.5));

    QVERIFY(oversampledAliasing < plainAliasing * 0.5);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AnalogFuzzTest)
