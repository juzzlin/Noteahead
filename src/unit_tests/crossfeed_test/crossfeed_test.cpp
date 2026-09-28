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

#include "crossfeed_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/crossfeed.hpp"
#include "../../domain/effects/crossfeed_presets.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

void setParameter(Crossfeed & effect, const QString & key, float value)
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

//! Runs a tone panned by @p pan (-1 left, 0 centre, 1 right) through the effect and returns both
//! outputs past the filters' warm-up.
Stereo render(Crossfeed & effect, double frequency, double pan, double amplitude = 0.5)
{
    effect.setSampleRate(SampleRate);

    const double leftGain = pan <= 0.0 ? 1.0 : 1.0 - pan;
    const double rightGain = pan >= 0.0 ? 1.0 : 1.0 + pan;

    Stereo out;
    const int total = 8192;
    const int warmup = 1024;
    for (int i = 0; i < total; i++) {
        const double sample = amplitude * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate);
        double left = sample * leftGain;
        double right = sample * rightGain;
        effect.process(left, right);
        if (i >= warmup) {
            out.left.push_back(left);
            out.right.push_back(right);
        }
    }
    return out;
}

double rms(const std::vector<double> & samples)
{
    double sum = 0.0;
    for (const double sample : samples) {
        sum += sample * sample;
    }
    return std::sqrt(sum / static_cast<double>(samples.size()));
}

//! How much of a hard-panned tone reaches the ear it was not panned to, in dB against the tone.
double bleedDb(Crossfeed & effect, double frequency)
{
    const auto out = render(effect, frequency, -1.0);
    return 20.0 * std::log10(rms(out.right) / (0.5 / std::numbers::sqrt2));
}

} // namespace

void CrossfeedTest::test_amountZero_shouldPassThrough()
{
    Crossfeed effect;
    setParameter(effect, Constants::NahdXml::xmlKeyAmount(), 0.0f);
    effect.setSampleRate(SampleRate);

    for (int i = 0; i < 512; i++) {
        const double sample = 0.4 * std::sin(2.0 * std::numbers::pi * 200.0 * static_cast<double>(i) / SampleRate);
        double left = sample;
        double right = -sample; // Pure side, which is the most the effect could possibly touch
        const double inLeft = left;
        const double inRight = right;
        effect.process(left, right);
        QVERIFY2(std::abs(left - inLeft) < 1.0e-9, qPrintable(QString { "Sample %1: %2 against %3" }.arg(i).arg(left).arg(inLeft)));
        QVERIFY2(std::abs(right - inRight) < 1.0e-9, qPrintable(QString { "Sample %1: %2 against %3" }.arg(i).arg(right).arg(inRight)));
    }
}

void CrossfeedTest::test_offlineBlock_shouldPassThrough()
{
    // The whole design: a monitoring aid must not print into an export.
    Crossfeed effect;
    effect.setSampleRate(SampleRate);

    constexpr uint32_t frameCount = 256;
    std::vector<double> buffer(frameCount * 2, 0.0);
    for (uint32_t i = 0; i < frameCount; i++) {
        buffer[i * 2] = 0.4 * std::sin(2.0 * std::numbers::pi * 200.0 * static_cast<double>(i) / SampleRate);
        buffer[i * 2 + 1] = -buffer[i * 2];
    }
    const auto reference = buffer;

    AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, static_cast<uint32_t>(SampleRate) };
    context.offline = true;
    effect.process(context);

    for (size_t i = 0; i < buffer.size(); i++) {
        QVERIFY2(std::abs(buffer[i] - reference[i]) < 1.0e-12, qPrintable(QString { "Sample %1 was touched by an offline render" }.arg(i)));
    }
}

void CrossfeedTest::test_hardPannedTone_shouldReachTheOtherEar()
{
    Crossfeed effect;
    effect.applyFactoryPreset(1); // Bauer

    const auto out = render(effect, 200.0, -1.0);

    // Panned hard left, so the right output is only what the crossfeed put there.
    QVERIFY2(rms(out.right) > 0.01, qPrintable(QString { "The far ear got %1" }.arg(rms(out.right))));
    // And what it gained, the near ear lost: nothing is created, it is moved.
    QVERIFY2(rms(out.left) < 0.5 / std::numbers::sqrt2, qPrintable(QString { "The near ear kept %1" }.arg(rms(out.left))));
}

void CrossfeedTest::test_hardPannedTone_shouldReachItDarkened()
{
    // The head shadows the far ear at high frequencies and hardly at all at low ones, which is what
    // makes this a crossfeed rather than a narrowing.
    Crossfeed effect;
    effect.applyFactoryPreset(1);
    const double low = bleedDb(effect, 200.0);

    Crossfeed treble;
    treble.applyFactoryPreset(1);
    const double high = bleedDb(treble, 6000.0);

    QVERIFY2(low - high > 15.0, qPrintable(QString { "200 Hz bled %1 dB and 6 kHz %2 dB" }.arg(low).arg(high)));
}

void CrossfeedTest::test_monoContent_shouldKeepItsLevel()
{
    // Centred material has no side signal to move, so it must come through untouched at every
    // frequency. Working on the sides rather than adding each channel to the other is what buys this.
    Crossfeed effect;
    effect.applyFactoryPreset(2); // Chu Moy, the strongest of the three

    for (const double frequency : { 50.0, 200.0, 1000.0, 6000.0 }) {
        const auto out = render(effect, frequency, 0.0);
        const double level = 20.0 * std::log10(rms(out.left) / (0.5 / std::numbers::sqrt2));
        QVERIFY2(std::abs(level) < 0.1, qPrintable(QString { "%1 Hz came out %2 dB off" }.arg(frequency).arg(level)));
    }
}

void CrossfeedTest::test_presets_shouldAllCrossfeedButOff()
{
    const auto & presets = CrossfeedPresets::presets();
    QCOMPARE(presets.front().name, std::string { "Off" });

    for (size_t i = 0; i < presets.size(); i++) {
        Crossfeed effect;
        effect.applyFactoryPreset(i);
        const double bleed = bleedDb(effect, 200.0);
        if (!i) {
            QVERIFY2(bleed < -80.0, qPrintable(QString { "Off bled %1 dB" }.arg(bleed)));
        } else {
            QVERIFY2(bleed > -20.0, qPrintable(QString { "%1 bled only %2 dB" }.arg(QString::fromStdString(presets[i].name)).arg(bleed)));
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::CrossfeedTest)
