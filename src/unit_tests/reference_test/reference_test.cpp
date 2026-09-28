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

#include "reference_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/reference.hpp"
#include "../../domain/effects/reference_environments.hpp"
#include "../../domain/effects/reference_presets.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;

void setParameter(Reference & effect, const QString & key, float value)
{
    if (auto parameter = effect.parameter(key.toStdString()); parameter) {
        parameter->get().update(value);
    }
    effect.sync();
}

//! Selects an environment by its place in the table, which is what the parameter stores.
void selectEnvironment(Reference & effect, const std::string & name)
{
    const auto & environments = referenceEnvironments();
    for (size_t i = 0; i < environments.size(); i++) {
        if (environments.at(i).name == name) {
            setParameter(effect, Constants::NahdXml::xmlKeyEnvironment(), static_cast<float>(i));
            return;
        }
    }
    QFAIL(qPrintable(QString { "No environment named %1" }.arg(QString::fromStdString(name))));
}

struct Stereo
{
    std::vector<double> left;
    std::vector<double> right;
};

Stereo render(Reference & effect, double frequency, double amplitude = 0.25, double pan = 0.0)
{
    effect.setSampleRate(SampleRate);

    const double leftGain = pan <= 0.0 ? 1.0 : 1.0 - pan;
    const double rightGain = pan >= 0.0 ? 1.0 : 1.0 + pan;

    Stereo out;
    const int total = 24000;
    const int warmup = 12000; // Past the compressor's attack and the longest reflection
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

//! Level of a tone after the effect, in dB against the tone itself.
double levelDb(Reference & effect, double frequency, double amplitude = 0.25)
{
    const auto out = render(effect, frequency, amplitude);
    return 20.0 * std::log10(rms(out.left) / (amplitude / std::numbers::sqrt2) + 1.0e-12);
}

} // namespace

void ReferenceTest::test_amountZero_shouldPassThrough()
{
    // Amount is the journey from this room to that one, so none of it has to be exactly this room.
    Reference effect;
    selectEnvironment(effect, "Phone");
    setParameter(effect, Constants::NahdXml::xmlKeyAmount(), 0.0f);
    effect.setSampleRate(SampleRate);

    for (int i = 0; i < 1024; i++) {
        const double sample = 0.3 * std::sin(2.0 * std::numbers::pi * 100.0 * static_cast<double>(i) / SampleRate);
        double left = sample;
        double right = sample * 0.5;
        const double inLeft = left;
        const double inRight = right;
        effect.process(left, right);
        QVERIFY2(std::abs(left - inLeft) < 1.0e-9, qPrintable(QString { "Sample %1: %2 against %3" }.arg(i).arg(left).arg(inLeft)));
        QVERIFY2(std::abs(right - inRight) < 1.0e-9, qPrintable(QString { "Sample %1: %2 against %3" }.arg(i).arg(right).arg(inRight)));
    }
}

void ReferenceTest::test_offlineBlock_shouldPassThrough()
{
    // The whole design: a reference check must never print into an export.
    Reference effect;
    selectEnvironment(effect, "Car");
    effect.setSampleRate(SampleRate);

    constexpr uint32_t frameCount = 512;
    std::vector<double> buffer(frameCount * 2, 0.0);
    for (uint32_t i = 0; i < frameCount; i++) {
        buffer[i * 2] = 0.3 * std::sin(2.0 * std::numbers::pi * 100.0 * static_cast<double>(i) / SampleRate);
        buffer[i * 2 + 1] = buffer[i * 2] * 0.5;
    }
    const auto reference = buffer;

    AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, static_cast<uint32_t>(SampleRate) };
    context.offline = true;
    effect.process(context);

    for (size_t i = 0; i < buffer.size(); i++) {
        QVERIFY2(std::abs(buffer[i] - reference[i]) < 1.0e-12, qPrintable(QString { "Sample %1 was touched by an offline render" }.arg(i)));
    }
}

void ReferenceTest::test_phone_shouldRemoveTheBottom()
{
    // What a phone speaker does most is refuse the bottom two octaves, which is the whole reason for
    // checking a mix on one.
    Reference effect;
    selectEnvironment(effect, "Phone");
    const double low = levelDb(effect, 60.0);

    Reference midrange;
    selectEnvironment(midrange, "Phone");
    const double mid = levelDb(midrange, 1500.0);

    QVERIFY2(mid - low > 30.0, qPrintable(QString { "60 Hz came out %1 dB and 1.5 kHz %2 dB" }.arg(low).arg(mid)));
}

void ReferenceTest::test_phone_shouldCollapseToMono()
{
    // One speaker, so nothing survives of what separates the channels.
    Reference effect;
    selectEnvironment(effect, "Phone");
    effect.setSampleRate(SampleRate);

    for (int i = 0; i < 4096; i++) {
        const double sample = 0.25 * std::sin(2.0 * std::numbers::pi * 1500.0 * static_cast<double>(i) / SampleRate);
        double left = sample;
        double right = -sample; // Pure side: a mono system must be left with nothing at all
        effect.process(left, right);
        if (i > 2048) {
            QVERIFY2(std::abs(left) < 1.0e-6, qPrintable(QString { "Sample %1 kept %2" }.arg(i).arg(left)));
        }
    }
}

void ReferenceTest::test_everyEnvironment_shouldChangeTheBalance()
{
    // Catches an environment that was added to the table and never filled in: every one of them has
    // to do something a mix decision could be made on.
    const auto & environments = referenceEnvironments();
    QVERIFY(!environments.empty());

    for (size_t i = 0; i < environments.size(); i++) {
        // Measured across the band rather than between two points: a voicing can happen to cross
        // flat at any given pair of frequencies without being flat anywhere else.
        double worst = 0.0;
        for (const double frequency : { 60.0, 200.0, 800.0, 3000.0, 9000.0 }) {
            Reference effect;
            setParameter(effect, Constants::NahdXml::xmlKeyEnvironment(), static_cast<float>(i));
            worst = std::max(worst, std::abs(levelDb(effect, frequency)));
        }
        // A low bar on purpose: what this catches is a table entry that was never filled in, and
        // Nearfield is meant to be nearly neutral -- being the honest one is its whole character.
        QVERIFY2(worst > 1.5,
                 qPrintable(QString { "%1 never departs from flat by more than %2 dB" }
                              .arg(QString::fromStdString(environments.at(i).name))
                              .arg(worst)));
    }
}

void ReferenceTest::test_everyEnvironment_shouldStaySane()
{
    // No environment may run away: these are all feedback-free, so anything above the input level by
    // more than the models' own boosts is a bug rather than a voicing.
    const auto & environments = referenceEnvironments();
    for (size_t i = 0; i < environments.size(); i++) {
        Reference effect;
        setParameter(effect, Constants::NahdXml::xmlKeyEnvironment(), static_cast<float>(i));
        effect.setSampleRate(SampleRate);

        double peak = 0.0;
        for (int n = 0; n < 24000; n++) {
            const double sample = 0.5 * std::sin(2.0 * std::numbers::pi * 220.0 * static_cast<double>(n) / SampleRate);
            double left = sample;
            double right = sample;
            effect.process(left, right);
            peak = std::max({ peak, std::abs(left), std::abs(right) });
            QVERIFY2(std::isfinite(left) && std::isfinite(right),
                     qPrintable(QString { "%1 produced a non-finite sample" }.arg(QString::fromStdString(environments.at(i).name))));
        }
        QVERIFY2(peak < 2.0, qPrintable(QString { "%1 peaked at %2" }.arg(QString::fromStdString(environments.at(i).name)).arg(peak)));
    }
}

void ReferenceTest::test_presets_shouldSelectTheirOwnEnvironment()
{
    // The list that selects an environment is built from the table of them, so a system added to the
    // table cannot be missing from the dropdown.
    const auto & presets = ReferencePresets::presets();
    const auto & environments = referenceEnvironments();
    QCOMPARE(presets.size(), environments.size());

    for (size_t i = 0; i < presets.size(); i++) {
        QCOMPARE(presets.at(i).name, environments.at(i).name);

        Reference effect;
        effect.applyFactoryPreset(i);
        QCOMPARE(effect.environment().name, environments.at(i).name);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ReferenceTest)
