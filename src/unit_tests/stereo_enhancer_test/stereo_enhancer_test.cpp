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

#include "stereo_enhancer_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/stereo_enhancer.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

//! Whole cycles per measurement, so every harmonic lands exactly on a DFT bin.
constexpr int Periods = 64;

void setParameter(StereoEnhancer & effect, const QString & key, float value)
{
    if (auto parameter = effect.parameter(key.toStdString()); parameter) {
        parameter->get().update(value);
    }
    effect.sync();
}

struct Stereo
{
    std::vector<double> left;
    std::vector<double> right;
};

//! Renders a sine, discarding a warm-up run first so the filters have settled before anything is
//! measured. The right channel can be given its own phase, which is what makes a stereo image.
Stereo renderSine(StereoEnhancer & effect, double frequency, double amplitude, double rightPhase = 0.0)
{
    effect.setSampleRate(SampleRate);

    const auto total = static_cast<size_t>(std::round(static_cast<double>(Periods) * SampleRate / frequency));

    const auto render = [&](Stereo * out) {
        for (size_t i = 0; i < total; i++) {
            const double phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate;
            double left = amplitude * std::sin(phase);
            double right = amplitude * std::sin(phase + rightPhase);
            effect.process(left, right);
            if (out) {
                out->left.push_back(left);
                out->right.push_back(right);
            }
        }
    };

    render(nullptr);
    Stereo out;
    render(&out);
    return out;
}

//! Magnitude at a frequency, by direct DFT probe.
double magnitudeAt(const std::vector<double> & samples, double frequency)
{
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        const double omega = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate;
        re += samples[i] * std::cos(omega);
        im -= samples[i] * std::sin(omega);
    }
    return std::hypot(re, im) / static_cast<double>(samples.size());
}

double rms(const std::vector<double> & samples)
{
    double sum = 0.0;
    for (const double sample : samples) {
        sum += sample * sample;
    }
    return std::sqrt(sum / static_cast<double>(samples.size()));
}

} // namespace

void StereoEnhancerTest::test_defaults_shouldPassSignalThrough()
{
    // Every band starts at zero, so a fresh instance has to be transparent.
    StereoEnhancer effect;
    effect.setSampleRate(SampleRate);

    for (int i = 0; i < 4096; i++) {
        const double sample = 0.4 * std::sin(2.0 * std::numbers::pi * 440.0 * i / SampleRate);
        double left = sample;
        double right = sample;
        effect.process(left, right);
        QVERIFY2(std::abs(left - sample) < 1.0e-9 && std::abs(right - sample) < 1.0e-9,
                 QString("Sample %1 changed with every band at zero: %2 against %3").arg(i).arg(left).arg(sample).toUtf8().constData());
    }
}

void StereoEnhancerTest::test_bass_shouldAddHarmonicsOfTheLowEnd()
{
    // What the bass control returns is harmonics of the low end rather than more low end, which is
    // what lets a small speaker imply a fundamental it cannot reproduce.
    constexpr double frequency = 80.0;

    StereoEnhancer flat;
    const auto reference = renderSine(flat, frequency, 0.5);

    StereoEnhancer enhanced;
    setParameter(enhanced, Constants::NahdXml::xmlKeyBassGain(), 1.0f);
    const auto output = renderSine(enhanced, frequency, 0.5);

    const double referenceSecond = magnitudeAt(reference.left, frequency * 2.0);
    const double outputSecond = magnitudeAt(output.left, frequency * 2.0);
    const double referenceThird = magnitudeAt(reference.left, frequency * 3.0);
    const double outputThird = magnitudeAt(output.left, frequency * 3.0);

    QVERIFY2(outputSecond > referenceSecond * 10.0 || outputThird > referenceThird * 10.0,
             QString("No harmonics were added: 2nd %1 against %2, 3rd %3 against %4")
               .arg(outputSecond)
               .arg(referenceSecond)
               .arg(outputThird)
               .arg(referenceThird)
               .toUtf8()
               .constData());
}

void StereoEnhancerTest::test_mid_shouldDipTheMidrange()
{
    constexpr double frequency = 1000.0;

    StereoEnhancer flat;
    const auto reference = renderSine(flat, frequency, 0.5);

    StereoEnhancer dipped;
    setParameter(dipped, Constants::NahdXml::xmlKeyMidGain(), 1.0f);
    const auto output = renderSine(dipped, frequency, 0.5);

    const double referenceLevel = magnitudeAt(reference.left, frequency);
    const double outputLevel = magnitudeAt(output.left, frequency);
    QVERIFY2(outputLevel < referenceLevel * 0.6,
             QString("The midrange was not dipped: %1 against %2").arg(outputLevel).arg(referenceLevel).toUtf8().constData());
}

void StereoEnhancerTest::test_high_shouldLiftTheTopEnd()
{
    constexpr double frequency = 6000.0;

    StereoEnhancer flat;
    const auto reference = renderSine(flat, frequency, 0.5);

    StereoEnhancer lifted;
    setParameter(lifted, Constants::NahdXml::xmlKeyHighGain(), 1.0f);
    const auto output = renderSine(lifted, frequency, 0.5);

    const double referenceLevel = magnitudeAt(reference.left, frequency);
    const double outputLevel = magnitudeAt(output.left, frequency);
    QVERIFY2(outputLevel > referenceLevel * 1.2,
             QString("The top end was not lifted: %1 against %2").arg(outputLevel).arg(referenceLevel).toUtf8().constData());
}

void StereoEnhancerTest::test_spread_shouldWidenTheSideSignal()
{
    // A quarter turn of phase between the channels gives the pair something off-centre to widen.
    constexpr double frequency = 1000.0;
    constexpr double phase = std::numbers::pi / 2.0;

    const auto sideRms = [](const Stereo & stereo) {
        std::vector<double> side(stereo.left.size(), 0.0);
        for (size_t i = 0; i < side.size(); i++) {
            side[i] = (stereo.left[i] - stereo.right[i]) * 0.5;
        }
        return rms(side);
    };

    StereoEnhancer flat;
    const auto reference = renderSine(flat, frequency, 0.4, phase);

    StereoEnhancer wide;
    setParameter(wide, Constants::NahdXml::xmlKeySpread(), 1.0f);
    const auto output = renderSine(wide, frequency, 0.4, phase);

    QVERIFY2(sideRms(output) > sideRms(reference) * 1.5,
             QString("The side signal was not widened: %1 against %2").arg(sideRms(output)).arg(sideRms(reference)).toUtf8().constData());
}

void StereoEnhancerTest::test_spread_monoInput_shouldStayMono()
{
    // Spread works on the side signal alone, so a mono source has nothing for it to widen and must
    // come out mono however far it is turned up.
    StereoEnhancer effect;
    setParameter(effect, Constants::NahdXml::xmlKeySpread(), 1.0f);
    const auto output = renderSine(effect, 1000.0, 0.4);

    for (size_t i = 0; i < output.left.size(); i++) {
        QVERIFY2(std::abs(output.left[i] - output.right[i]) < 1.0e-9,
                 QString("Sample %1 was pushed off centre: %2 against %3").arg(i).arg(output.left[i]).arg(output.right[i]).toUtf8().constData());
    }
}

void StereoEnhancerTest::test_outGain_shouldScaleOutput()
{
    StereoEnhancer flat;
    const auto reference = renderSine(flat, 1000.0, 0.3);

    StereoEnhancer louder;
    setParameter(louder, Constants::NahdXml::xmlKeyGain(), 1.0f); // +12 dB
    const auto output = renderSine(louder, 1000.0, 0.3);

    const double ratio = rms(output.left) / rms(reference.left);
    QVERIFY2(std::abs(ratio - 3.98) < 0.1,
             QString("Full out gain gave %1x, expected about 4x").arg(ratio).toUtf8().constData());
}

void StereoEnhancerTest::test_mix_zero_shouldPassSignalThrough()
{
    StereoEnhancer effect;
    setParameter(effect, Constants::NahdXml::xmlKeyBassGain(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyHighGain(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyGain(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);
    effect.setSampleRate(SampleRate);

    for (int i = 0; i < 4096; i++) {
        const double sample = 0.4 * std::sin(2.0 * std::numbers::pi * 200.0 * i / SampleRate);
        double left = sample;
        double right = sample;
        effect.process(left, right);
        QVERIFY2(std::abs(left - sample) < 1.0e-9,
                 QString("Sample %1 changed with the mix fully dry: %2 against %3").arg(i).arg(left).arg(sample).toUtf8().constData());
    }
}

void StereoEnhancerTest::test_solo_shouldPassOnlyWhatTheEffectAdds()
{
    // Solo is the difference between what came in and what goes out, so soloed output plus dry has
    // to reconstruct the ordinary output exactly.
    constexpr double frequency = 80.0;

    StereoEnhancer ordinary;
    setParameter(ordinary, Constants::NahdXml::xmlKeyBassGain(), 1.0f);
    setParameter(ordinary, Constants::NahdXml::xmlKeyHighGain(), 0.7f);
    const auto wet = renderSine(ordinary, frequency, 0.5);

    StereoEnhancer soloed;
    setParameter(soloed, Constants::NahdXml::xmlKeyBassGain(), 1.0f);
    setParameter(soloed, Constants::NahdXml::xmlKeyHighGain(), 0.7f);
    setParameter(soloed, Constants::NahdXml::xmlKeySolo(), 1.0f);
    const auto difference = renderSine(soloed, frequency, 0.5);

    QVERIFY(rms(difference.left) > 0.0);

    for (size_t i = 0; i < wet.left.size(); i++) {
        const double phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate;
        const double dry = 0.5 * std::sin(phase);
        QVERIFY2(std::abs((difference.left[i] + dry) - wet.left[i]) < 1.0e-9,
                 QString("Sample %1 did not reconstruct: %2 + %3 against %4").arg(i).arg(difference.left[i]).arg(dry).arg(wet.left[i]).toUtf8().constData());
    }
}

void StereoEnhancerTest::test_solo_blockPath_shouldMatchTheSamplePath()
{
    // The rack calls effects both a frame at a time and a block at a time, and Solo has to mean the
    // same thing either way: the block path has to keep its own copy of the dry signal to subtract.
    const auto renderBlock = [](bool solo) {
        StereoEnhancer effect;
        effect.setSampleRate(SampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyBassGain(), 1.0f);
        if (solo) {
            setParameter(effect, Constants::NahdXml::xmlKeySolo(), 1.0f);
        }
        constexpr uint32_t frames = 2048;
        std::vector<double> buffer(frames * 2, 0.0);
        for (uint32_t i = 0; i < frames; i++) {
            const double sample = 0.5 * std::sin(2.0 * std::numbers::pi * 80.0 * i / SampleRate);
            buffer[i * 2] = sample;
            buffer[i * 2 + 1] = sample;
        }
        AudioContext context { std::span(buffer.data(), buffer.size()), frames, static_cast<uint32_t>(SampleRate) };
        effect.process(context);
        return buffer;
    };

    const auto wet = renderBlock(false);
    const auto difference = renderBlock(true);

    for (uint32_t i = 0; i < 2048; i++) {
        const double dry = 0.5 * std::sin(2.0 * std::numbers::pi * 80.0 * i / SampleRate);
        QVERIFY2(std::abs((difference[i * 2] + dry) - wet[i * 2]) < 1.0e-9,
                 QString("Frame %1 did not reconstruct through the block path").arg(i).toUtf8().constData());
    }
}

void StereoEnhancerTest::test_solo_transparentEffect_shouldFallSilent()
{
    // With every band at zero the enhancer adds nothing, so there is nothing for Solo to pass.
    StereoEnhancer effect;
    setParameter(effect, Constants::NahdXml::xmlKeySolo(), 1.0f);
    const auto output = renderSine(effect, 1000.0, 0.5);

    QVERIFY2(rms(output.left) < 1.0e-9,
             QString("Solo passed %1 from an effect that adds nothing").arg(rms(output.left)).toUtf8().constData());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StereoEnhancerTest)
