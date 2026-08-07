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

#include "stereo_exciter_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/stereo_exciter.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

//! Whole cycles per measurement, so every harmonic lands exactly on a DFT bin.
constexpr int Periods = 64;

void setParameter(StereoExciter & effect, const QString & key, float value)
{
    if (auto parameter = effect.parameter(key.toStdString()); parameter) {
        parameter->get().update(value);
    }
    effect.sync();
}

//! Renders a sine, discarding a warm-up run first so the filters have settled.
std::vector<double> renderSine(StereoExciter & effect, double frequency, double amplitude)
{
    effect.setSampleRate(SampleRate);

    const auto total = static_cast<size_t>(std::round(static_cast<double>(Periods) * SampleRate / frequency));

    const auto render = [&](std::vector<double> * out) {
        for (size_t i = 0; i < total; i++) {
            const double phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate;
            double left = amplitude * std::sin(phase);
            double right = left;
            effect.process(left, right);
            if (out) {
                out->push_back(left);
            }
        }
    };

    render(nullptr);
    std::vector<double> out;
    render(&out);
    return out;
}

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

//! Timbre is bipolar around the centre of its range: one end odd, the other even.
float timbre(float amount)
{
    return 0.5f + amount * 0.5f;
}

} // namespace

void StereoExciterTest::test_harmonicsZero_shouldPassSignalThrough()
{
    StereoExciter effect;
    effect.setSampleRate(SampleRate);

    for (int i = 0; i < 4096; i++) {
        const double sample = 0.4 * std::sin(2.0 * std::numbers::pi * 3000.0 * i / SampleRate);
        double left = sample;
        double right = sample;
        effect.process(left, right);
        QVERIFY2(std::abs(left - sample) < 1.0e-9,
                 QString("Sample %1 changed with Harmonics at zero: %2 against %3").arg(i).arg(left).arg(sample).toUtf8().constData());
    }
}

void StereoExciterTest::test_harmonics_shouldGenerateContentAboveTheInput()
{
    // The point of an exciter: it makes top end where an equalizer would have nothing to lift.
    constexpr double frequency = 3000.0;

    StereoExciter flat;
    const auto reference = renderSine(flat, frequency, 0.5);

    StereoExciter excited;
    setParameter(excited, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    const auto output = renderSine(excited, frequency, 0.5);

    const double referenceSecond = magnitudeAt(reference, frequency * 2.0);
    const double outputSecond = magnitudeAt(output, frequency * 2.0);
    const double referenceThird = magnitudeAt(reference, frequency * 3.0);
    const double outputThird = magnitudeAt(output, frequency * 3.0);

    QVERIFY2(outputSecond > referenceSecond * 10.0 || outputThird > referenceThird * 10.0,
             QString("No harmonics were generated: 2nd %1 against %2, 3rd %3 against %4")
               .arg(outputSecond)
               .arg(referenceSecond)
               .arg(outputThird)
               .arg(referenceThird)
               .toUtf8()
               .constData());
}

void StereoExciterTest::test_belowTune_shouldBeLeftAlone()
{
    // Only the band above Tune feeds the shaper, so a low tone must come through essentially as it
    // went in: an exciter that muddied the bottom would be doing the opposite of its job.
    constexpr double frequency = 100.0;

    StereoExciter effect;
    setParameter(effect, Constants::NahdXml::xmlKeyTune(), 1.0f); // 8 kHz
    setParameter(effect, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    const auto output = renderSine(effect, frequency, 0.5);

    const double fundamental = magnitudeAt(output, frequency);
    const double second = magnitudeAt(output, frequency * 2.0);
    QVERIFY2(second < fundamental * 0.05,
             QString("A tone well below Tune was distorted: 2nd %1 against fundamental %2").arg(second).arg(fundamental).toUtf8().constData());
}

void StereoExciterTest::test_timbre_odd_shouldFavourOddHarmonics()
{
    constexpr double frequency = 2000.0;

    StereoExciter effect;
    setParameter(effect, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyTimbre(), timbre(-1.0f));
    const auto output = renderSine(effect, frequency, 0.5);

    const double second = magnitudeAt(output, frequency * 2.0);
    const double third = magnitudeAt(output, frequency * 3.0);
    QVERIFY2(third > second,
             QString("Odd timbre did not favour odd harmonics: 3rd %1 against 2nd %2").arg(third).arg(second).toUtf8().constData());
}

void StereoExciterTest::test_timbre_even_shouldFavourEvenHarmonics()
{
    constexpr double frequency = 2000.0;

    StereoExciter odd;
    setParameter(odd, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    setParameter(odd, Constants::NahdXml::xmlKeyTimbre(), timbre(-1.0f));
    const auto oddOutput = renderSine(odd, frequency, 0.5);

    StereoExciter even;
    setParameter(even, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    setParameter(even, Constants::NahdXml::xmlKeyTimbre(), timbre(1.0f));
    const auto evenOutput = renderSine(even, frequency, 0.5);

    const double oddSecond = magnitudeAt(oddOutput, frequency * 2.0);
    const double evenSecond = magnitudeAt(evenOutput, frequency * 2.0);
    QVERIFY2(evenSecond > oddSecond * 2.0,
             QString("Even timbre did not add second-harmonic content: %1 against %2").arg(evenSecond).arg(oddSecond).toUtf8().constData());
}

void StereoExciterTest::test_solo_shouldPassOnlyTheHarmonics()
{
    // Solo has to leave exactly what the exciter contributes, so soloed plus dry reconstructs the
    // ordinary output.
    constexpr double frequency = 3000.0;

    StereoExciter ordinary;
    setParameter(ordinary, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    const auto wet = renderSine(ordinary, frequency, 0.5);

    StereoExciter soloed;
    setParameter(soloed, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    setParameter(soloed, Constants::NahdXml::xmlKeySolo(), 1.0f);
    const auto difference = renderSine(soloed, frequency, 0.5);

    QVERIFY(rms(difference) > 0.0);

    for (size_t i = 0; i < wet.size(); i++) {
        const double dry = 0.5 * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate);
        QVERIFY2(std::abs((difference[i] + dry) - wet[i]) < 1.0e-9,
                 QString("Sample %1 did not reconstruct: %2 + %3 against %4").arg(i).arg(difference[i]).arg(dry).arg(wet[i]).toUtf8().constData());
    }
}

void StereoExciterTest::test_mix_zero_shouldPassSignalThrough()
{
    StereoExciter effect;
    setParameter(effect, Constants::NahdXml::xmlKeyHarmonics(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);
    effect.setSampleRate(SampleRate);

    for (int i = 0; i < 4096; i++) {
        const double sample = 0.4 * std::sin(2.0 * std::numbers::pi * 3000.0 * i / SampleRate);
        double left = sample;
        double right = sample;
        effect.process(left, right);
        QVERIFY2(std::abs(left - sample) < 1.0e-9,
                 QString("Sample %1 changed with the mix fully dry: %2 against %3").arg(i).arg(left).arg(sample).toUtf8().constData());
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::StereoExciterTest)
