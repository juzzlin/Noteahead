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

#include "wave_designer_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/effects/wave_designer.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

void setParameter(WaveDesigner & effect, const QString & key, float value)
{
    if (auto parameter = effect.parameter(key.toStdString()); parameter) {
        parameter->get().update(value);
    }
    effect.sync();
}

//! A drum-like hit: an instant edge followed by an exponential decay, which is the shape both
//! controls are meant to act on.
std::vector<double> hit(double amplitude, double decayMs, double lengthMs, double silenceMs = 250.0)
{
    const auto silenceLength = static_cast<size_t>(silenceMs * SampleRate / 1000.0);
    const auto hitLength = static_cast<size_t>(lengthMs * SampleRate / 1000.0);
    std::vector<double> samples(silenceLength + hitLength, 0.0);
    const double decay = std::exp(-1.0 / (decayMs * SampleRate / 1000.0));
    double envelope = amplitude;
    for (size_t i = silenceLength; i < samples.size(); i++) {
        // 300 Hz carrier: high enough to be audio, low enough that the followers see the envelope.
        samples[i] = envelope * std::sin(2.0 * std::numbers::pi * 300.0 * static_cast<double>(i - silenceLength) / SampleRate);
        envelope *= decay;
    }
    return samples;
}

std::vector<double> render(WaveDesigner & effect, const std::vector<double> & input)
{
    effect.setSampleRate(SampleRate);
    effect.reset();
    std::vector<double> output(input.size(), 0.0);
    for (size_t i = 0; i < input.size(); i++) {
        double left = input[i];
        double right = input[i];
        effect.process(left, right);
        output[i] = left;
    }
    return output;
}

//! Peak over a window given in milliseconds from the start of the hit.
double peakBetween(const std::vector<double> & samples, double fromMs, double toMs, double silenceMs = 250.0)
{
    const auto begin = static_cast<size_t>((silenceMs + fromMs) * SampleRate / 1000.0);
    const auto end = std::min(samples.size(), static_cast<size_t>((silenceMs + toMs) * SampleRate / 1000.0));
    double peak = 0.0;
    for (size_t i = begin; i < end; i++) {
        peak = std::max(peak, std::abs(samples[i]));
    }
    return peak;
}

//! Attack and Sustain are bipolar around the centre of the parameter's range.
float bipolar(float amount)
{
    return 0.5f + amount * 0.5f;
}

} // namespace

void WaveDesignerTest::test_centred_shouldPassSignalThrough()
{
    WaveDesigner effect;
    const auto input = hit(0.5, 60.0, 400.0);
    const auto output = render(effect, input);

    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY2(std::abs(output[i] - input[i]) < 1.0e-6,
                 QString("Sample %1 changed with every control centred: %2 against %3").arg(i).arg(output[i]).arg(input[i]).toUtf8().constData());
    }
}

void WaveDesignerTest::test_attack_positive_shouldEmphasiseTheLeadingEdge()
{
    const auto input = hit(0.4, 60.0, 400.0);

    WaveDesigner flat;
    const auto reference = render(flat, input);

    WaveDesigner shaped;
    setParameter(shaped, Constants::NahdXml::xmlKeyAttack(), bipolar(1.0f));
    const auto output = render(shaped, input);

    const double referenceEdge = peakBetween(reference, 0.0, 10.0);
    const double shapedEdge = peakBetween(output, 0.0, 10.0);
    QVERIFY2(shapedEdge > referenceEdge * 1.5,
             QString("The edge was not emphasised: %1 against %2").arg(shapedEdge).arg(referenceEdge).toUtf8().constData());

    // The tail is the Sustain control's business, so it has to come through roughly untouched.
    const double referenceTail = peakBetween(reference, 200.0, 260.0);
    const double shapedTail = peakBetween(output, 200.0, 260.0);
    QVERIFY2(shapedTail < referenceTail * 1.3,
             QString("Attack also lifted the tail: %1 against %2").arg(shapedTail).arg(referenceTail).toUtf8().constData());
}

void WaveDesignerTest::test_attack_negative_shouldTameTheLeadingEdge()
{
    const auto input = hit(0.4, 60.0, 400.0);

    WaveDesigner flat;
    const auto reference = render(flat, input);

    WaveDesigner shaped;
    setParameter(shaped, Constants::NahdXml::xmlKeyAttack(), bipolar(-1.0f));
    const auto output = render(shaped, input);

    const double referenceEdge = peakBetween(reference, 0.0, 10.0);
    const double shapedEdge = peakBetween(output, 0.0, 10.0);
    QVERIFY2(shapedEdge < referenceEdge * 0.7,
             QString("The edge was not tamed: %1 against %2").arg(shapedEdge).arg(referenceEdge).toUtf8().constData());
}

void WaveDesignerTest::test_sustain_positive_shouldHoldTheTailUp()
{
    const auto input = hit(0.4, 60.0, 400.0);

    WaveDesigner flat;
    const auto reference = render(flat, input);

    WaveDesigner shaped;
    setParameter(shaped, Constants::NahdXml::xmlKeySustain(), bipolar(1.0f));
    const auto output = render(shaped, input);

    const double referenceTail = peakBetween(reference, 120.0, 260.0);
    const double shapedTail = peakBetween(output, 120.0, 260.0);
    QVERIFY2(shapedTail > referenceTail * 1.3,
             QString("The tail was not held up: %1 against %2").arg(shapedTail).arg(referenceTail).toUtf8().constData());
}

void WaveDesignerTest::test_sustain_negative_shouldShortenTheTail()
{
    const auto input = hit(0.4, 60.0, 400.0);

    WaveDesigner flat;
    const auto reference = render(flat, input);

    WaveDesigner shaped;
    setParameter(shaped, Constants::NahdXml::xmlKeySustain(), bipolar(-1.0f));
    const auto output = render(shaped, input);

    const double referenceTail = peakBetween(reference, 120.0, 260.0);
    const double shapedTail = peakBetween(output, 120.0, 260.0);
    QVERIFY2(shapedTail < referenceTail * 0.8,
             QString("The tail was not shortened: %1 against %2").arg(shapedTail).arg(referenceTail).toUtf8().constData());
}

void WaveDesignerTest::test_steadyTone_shouldNotBeShaped()
{
    // Nothing about a held tone is a transient, so both followers agree and the shaper has to leave
    // it alone. This is what separates it from a compressor.
    // Two seconds, measured over the last half: the slowest follower releases over 900 ms, so a
    // tone has to be held for a while before "steady" is true of every follower at once.
    const auto length = static_cast<size_t>(SampleRate * 2);
    std::vector<double> input(length, 0.0);
    for (size_t i = 0; i < length; i++) {
        input[i] = 0.4 * std::sin(2.0 * std::numbers::pi * 300.0 * static_cast<double>(i) / SampleRate);
    }

    WaveDesigner effect;
    setParameter(effect, Constants::NahdXml::xmlKeyAttack(), bipolar(1.0f));
    setParameter(effect, Constants::NahdXml::xmlKeySustain(), bipolar(1.0f));
    const auto output = render(effect, input);

    // Skip the onset of the tone itself, which is a transient like any other.
    const auto begin = static_cast<size_t>(SampleRate * 1.5);
    double peakRatio = 0.0;
    for (size_t i = begin; i < length; i++) {
        if (std::abs(input[i]) > 0.05) {
            peakRatio = std::max(peakRatio, std::abs(output[i] / input[i]));
        }
    }
    QVERIFY2(peakRatio < 1.15,
             QString("A held tone was shaped by up to %1x").arg(peakRatio).toUtf8().constData());
}

void WaveDesignerTest::test_shaping_shouldBeLevelIndependent()
{
    // The same hit 20 dB down has to be shaped by the same amount, which is what the followers being
    // compared in dB buys.
    WaveDesigner loud;
    setParameter(loud, Constants::NahdXml::xmlKeyAttack(), bipolar(1.0f));
    const auto loudInput = hit(0.5, 60.0, 400.0);
    const auto loudOutput = render(loud, loudInput);

    WaveDesigner quiet;
    setParameter(quiet, Constants::NahdXml::xmlKeyAttack(), bipolar(1.0f));
    const auto quietInput = hit(0.05, 60.0, 400.0);
    const auto quietOutput = render(quiet, quietInput);

    const double loudGain = peakBetween(loudOutput, 0.0, 10.0) / peakBetween(loudInput, 0.0, 10.0);
    const double quietGain = peakBetween(quietOutput, 0.0, 10.0) / peakBetween(quietInput, 0.0, 10.0);

    QVERIFY2(std::abs(loudGain - quietGain) < loudGain * 0.1,
             QString("Shaping followed the level: %1x loud against %2x quiet").arg(loudGain).arg(quietGain).toUtf8().constData());
}

void WaveDesignerTest::test_gain_shouldScaleOutput()
{
    const auto input = hit(0.3, 60.0, 200.0);

    WaveDesigner flat;
    const auto reference = render(flat, input);

    WaveDesigner louder;
    setParameter(louder, Constants::NahdXml::xmlKeyGain(), 1.0f); // +24 dB
    const auto output = render(louder, input);

    const double ratio = peakBetween(output, 0.0, 150.0) / peakBetween(reference, 0.0, 150.0);
    QVERIFY2(std::abs(ratio - 15.85) < 0.5,
             QString("Full gain gave %1x, expected about 16x").arg(ratio).toUtf8().constData());
}

void WaveDesignerTest::test_mix_zero_shouldPassSignalThrough()
{
    WaveDesigner effect;
    setParameter(effect, Constants::NahdXml::xmlKeyAttack(), bipolar(1.0f));
    setParameter(effect, Constants::NahdXml::xmlKeyGain(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);

    const auto input = hit(0.4, 60.0, 200.0);
    const auto output = render(effect, input);

    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY2(std::abs(output[i] - input[i]) < 1.0e-9,
                 QString("Sample %1 changed with the mix fully dry: %2 against %3").arg(i).arg(output[i]).arg(input[i]).toUtf8().constData());
    }
}

void WaveDesignerTest::test_reset_shouldClearFollowers()
{
    WaveDesigner effect;
    setParameter(effect, Constants::NahdXml::xmlKeyAttack(), bipolar(1.0f));

    const auto input = hit(0.5, 60.0, 400.0);
    const auto first = render(effect, input);
    const auto second = render(effect, input);

    for (size_t i = 0; i < first.size(); i++) {
        QVERIFY2(std::abs(first[i] - second[i]) < 1.0e-9,
                 QString("Sample %1 differed after a reset: %2 against %3").arg(i).arg(first[i]).arg(second[i]).toUtf8().constData());
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WaveDesignerTest)
