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

#include "effect_oversampling_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/effects/analog_fuzz.hpp"
#include "../../domain/effects/bass_grinder.hpp"
#include "../../domain/effects/clipper.hpp"
#include "../../domain/effects/drive.hpp"
#include "../../domain/effects/saturator.hpp"
#include "../../domain/effects/stereo_exciter.hpp"
#include "../../domain/effects/tube_stage.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

void setParam(Effect & effect, const QString & key, float value)
{
    if (auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
}

// Single-bin DFT magnitude (Goertzel) at a normalised frequency (cycles/sample).
double goertzel(const std::vector<double> & signal, double frequency)
{
    const double w = 2.0 * std::numbers::pi * frequency;
    const double coeff = 2.0 * std::cos(w);
    double s1 = 0.0;
    double s2 = 0.0;
    for (const double v : signal) {
        const double s0 = v + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const double real = s1 - s2 * std::cos(w);
    const double imag = s2 * std::sin(w);
    return std::sqrt(real * real + imag * imag);
}

// Drives a hot sine through the effect and returns the magnitude at the alias frequency. With no
// oversampling the shaper's harmonics fold back to aliasFreq; with oversampling they are filtered
// out before decimation, so the magnitude drops sharply.
double aliasMagnitude(Effect & effect, uint8_t factor, double inputFreq, double aliasFreq, double amplitude)
{
    effect.reset();
    effect.setOversampleFactor(factor);

    const int total = 4096;
    const int warmup = 512;
    std::vector<double> out;
    out.reserve(total - warmup);
    for (int i = 0; i < total; i++) {
        const double x = amplitude * std::sin(2.0 * std::numbers::pi * inputFreq * static_cast<double>(i));
        double l = x;
        double r = x;
        effect.process(l, r);
        if (i >= warmup) {
            out.push_back(l);
        }
    }
    return goertzel(out, aliasFreq);
}

// Level of a quiet sine at the input frequency after the effect, in dB. Quiet so that the effect stays
// close to linear and what is measured is the path the signal takes, not the distortion.
double levelDb(Effect & effect, uint8_t factor, double frequency, double amplitude = 0.05)
{
    effect.reset();
    effect.setOversampleFactor(factor);

    const int total = 8192;
    const int warmup = 1024;
    std::vector<double> out;
    out.reserve(total - warmup);
    for (int i = 0; i < total; i++) {
        double l = amplitude * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i));
        double r = l;
        effect.process(l, r);
        if (i >= warmup) {
            out.push_back(l);
        }
    }
    return 20.0 * std::log10(goertzel(out, frequency));
}

// Oversampling delays whatever goes through the resampler, so a dry or cancelling path that skips
// it combs against the wet one. The level at 2x and 4x must then match 1x across the band.
void verifyLevelKeptAcrossFactors(Effect & effect, double amplitude = 0.05)
{
    for (const double frequency : { 0.0125, 0.025, 0.05, 0.1 }) {
        const double reference = levelDb(effect, 1, frequency, amplitude);
        for (const uint8_t factor : { uint8_t { 2 }, uint8_t { 4 } }) {
            const double level = levelDb(effect, factor, frequency, amplitude);
            QVERIFY2(std::abs(level - reference) < 0.5, qPrintable(QString { "%1x at %2: %3 dB vs %4 dB at 1x" }.arg(factor).arg(frequency).arg(level).arg(reference)));
        }
    }
}

} // namespace

void EffectOversamplingTest::test_clipper_higherFactor_shouldReduceAliasing()
{
    Clipper clipper;
    setParam(clipper, Constants::NahdXml::xmlKeyMode(), 0.0f); // Hard clip
    setParam(clipper, Constants::NahdXml::xmlKeyThreshold(), 0.5f);
    clipper.sync();

    // 3rd harmonic of 0.3 (=0.9) aliases to 0.1.
    const double mag1 = aliasMagnitude(clipper, 1, 0.3, 0.1, 0.7);
    const double mag4 = aliasMagnitude(clipper, 4, 0.3, 0.1, 0.7);

    QVERIFY(mag1 > 1.0); // Aliasing is clearly present without oversampling
    QVERIFY(mag4 < 0.25 * mag1); // 4x oversampling substantially reduces it
}

void EffectOversamplingTest::test_drive_higherFactor_shouldReduceAliasing()
{
    Drive drive;
    setParam(drive, Constants::NahdXml::xmlKeyMode(), 1.0f); // Hard
    setParam(drive, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    setParam(drive, Constants::NahdXml::xmlKeyMix(), 1.0f);
    drive.sync();

    const double mag1 = aliasMagnitude(drive, 1, 0.3, 0.1, 0.5);
    const double mag4 = aliasMagnitude(drive, 4, 0.3, 0.1, 0.5);

    QVERIFY(mag1 > 1.0);
    QVERIFY(mag4 < 0.25 * mag1);
}

void EffectOversamplingTest::test_saturator_higherFactor_shouldReduceAliasing()
{
    Saturator saturator;
    setParam(saturator, Constants::NahdXml::xmlKeyMode(), 0.0f); // Tape (tanh)
    setParam(saturator, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    setParam(saturator, Constants::NahdXml::xmlKeyTone(), 1.0f);
    setParam(saturator, Constants::NahdXml::xmlKeyMix(), 1.0f);
    saturator.sync();

    const double mag1 = aliasMagnitude(saturator, 1, 0.3, 0.1, 0.8);
    const double mag4 = aliasMagnitude(saturator, 4, 0.3, 0.1, 0.8);

    QVERIFY(mag1 > 0.5);
    QVERIFY(mag4 < 0.6 * mag1);
}

void EffectOversamplingTest::test_bassGrinder_higherFactor_shouldReduceAliasing()
{
    BassGrinder bassGrinder;
    setParam(bassGrinder, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    setParam(bassGrinder, Constants::NahdXml::xmlKeyBlend(), 1.0f);
    setParam(bassGrinder, Constants::NahdXml::xmlKeySplitFreq(), 0.0f); // Split below the band, so everything is clipped
    setParam(bassGrinder, Constants::NahdXml::xmlKeyMix(), 1.0f);
    bassGrinder.sync();

    const double mag1 = aliasMagnitude(bassGrinder, 1, 0.3, 0.1, 0.7);
    const double mag4 = aliasMagnitude(bassGrinder, 4, 0.3, 0.1, 0.7);

    QVERIFY(mag1 > 0.5);
    QVERIFY(mag4 < 0.25 * mag1);
}

void EffectOversamplingTest::test_drive_factorOne_dryMix_shouldPassThrough()
{
    Drive drive;
    setParam(drive, Constants::NahdXml::xmlKeyDrive(), 0.9f);
    setParam(drive, Constants::NahdXml::xmlKeyMix(), 0.0f); // Fully dry
    drive.sync();
    drive.setOversampleFactor(1);

    // A fully dry, unity-gain Drive must pass the signal through untouched.
    for (int i = 0; i < 16; i++) {
        double l = 0.25 * static_cast<double>(i) - 0.5;
        double r = -l;
        const double inL = l;
        const double inR = r;
        drive.process(l, r);
        QVERIFY(std::abs(l - inL) < 1.0e-6);
        QVERIFY(std::abs(r - inR) < 1.0e-6);
    }
}

void EffectOversamplingTest::test_stereoExciter_higherFactor_shouldKeepLevel()
{
    StereoExciter exciter;
    setParam(exciter, Constants::NahdXml::xmlKeyTune(), 0.0f); // Lowest corner, so the band covers the test tones
    setParam(exciter, Constants::NahdXml::xmlKeyZeroFill(), 1.0f);
    setParam(exciter, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    exciter.sync();

    verifyLevelKeptAcrossFactors(exciter);
}

void EffectOversamplingTest::test_stereoExciter_partialMix_higherFactor_shouldKeepLevel()
{
    // Mix is blended by the base class against the input as it came in, so whatever the exciter
    // passes through has to arrive without the resampling latency or the blend combs.
    StereoExciter exciter;
    setParam(exciter, Constants::NahdXml::xmlKeyTune(), 0.0f);
    setParam(exciter, Constants::NahdXml::xmlKeyZeroFill(), 1.0f);
    setParam(exciter, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    setParam(exciter, Constants::NahdXml::xmlKeyMix(), 0.7f);
    exciter.sync();

    verifyLevelKeptAcrossFactors(exciter);
}

void EffectOversamplingTest::test_stereoExciter_solo_higherFactor_shouldPassOnlyHarmonics()
{
    // Solo is the output minus the input. A quiet tone barely excites the shaper, so what is left
    // must be far below the tone itself at every factor, not a delayed copy of it.
    StereoExciter exciter;
    setParam(exciter, Constants::NahdXml::xmlKeyTune(), 0.0f);
    setParam(exciter, Constants::NahdXml::xmlKeyZeroFill(), 1.0f);
    setParam(exciter, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    setParam(exciter, Constants::NahdXml::xmlKeySolo(), 1.0f);
    exciter.sync();

    const double input = 20.0 * std::log10(0.05 * 7168 / 2); // Goertzel magnitude of the unprocessed tone
    for (const double frequency : { 0.0125, 0.025, 0.05, 0.1 }) {
        for (const uint8_t factor : { uint8_t { 1 }, uint8_t { 2 }, uint8_t { 4 } }) {
            const double level = levelDb(exciter, factor, frequency);
            QVERIFY2(level < input - 10.0, qPrintable(QString { "%1x at %2: %3 dB against %4 dB in" }.arg(factor).arg(frequency).arg(level).arg(input)));
        }
    }
}

void EffectOversamplingTest::test_tubeStage_partialMix_higherFactor_shouldKeepLevel()
{
    TubeStage tubeStage;
    setParam(tubeStage, Constants::NahdXml::xmlKeyMix(), 0.5f);
    tubeStage.sync();

    verifyLevelKeptAcrossFactors(tubeStage);
}

void EffectOversamplingTest::test_analogFuzz_partialMix_higherFactor_shouldKeepLevel()
{
    AnalogFuzz analogFuzz;
    setParam(analogFuzz, Constants::NahdXml::xmlKeyMix(), 0.5f);
    analogFuzz.sync();

    verifyLevelKeptAcrossFactors(analogFuzz);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EffectOversamplingTest)
