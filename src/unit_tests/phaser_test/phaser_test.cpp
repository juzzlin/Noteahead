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

#include "phaser_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/lfo.hpp"
#include "../../domain/effects/phaser.hpp"

#include <QTest>

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double testSampleRate = 44100.0;

//! Window the RMS readings are taken over. Every test tone fits a whole number of cycles into it,
//! so a window's reading only moves when the sweep does.
constexpr size_t wholeCycleWindow = 441;

constexpr double unitSineRms = 0.70710678;

//! Internal value that leaves a bipolar parameter at its centre.
constexpr float noFeedback = 0.5f;

void setParameter(Phaser & effect, const QString & key, float value)
{
    const auto parameter = effect.parameter(key.toStdString());
    QVERIFY(parameter.has_value());
    parameter->get().setValue(value);
}

//! Rate that gives the sweep the requested frequency in its Hz mode, inverting the curve
//! ParameterMapper::mapLfoFrequency puts the rate knob on.
float rateForHz(double hz)
{
    return static_cast<float>(std::log(hz + 0.95) / std::log(20.0));
}

double sineAt(double frequency, size_t index)
{
    return std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(index) / testSampleRate);
}

//! A phaser holding still: no sweep, so its notches stay where the centre frequency puts them.
std::unique_ptr<Phaser> makeStaticPhaser(int stages, float mix, float feedback = noFeedback)
{
    auto effect = std::make_unique<Phaser>();
    effect->setSampleRate(testSampleRate);
    setParameter(*effect, Constants::NahdXml::xmlKeyStages(), static_cast<float>(stages));
    setParameter(*effect, Constants::NahdXml::xmlKeyDepth(), 0.0f);
    setParameter(*effect, Constants::NahdXml::xmlKeyFeedback(), feedback);
    setParameter(*effect, Constants::NahdXml::xmlKeyMix(), mix);
    effect->sync();
    return effect;
}

//! Output level at one frequency, relative to the unit sine that went in.
double renderGainDb(Phaser & effect, double frequency, double seconds = 0.15)
{
    const auto sampleCount = static_cast<size_t>(testSampleRate * seconds);
    const auto skipCount = sampleCount / 2;
    double sum = 0.0;
    size_t counted = 0;
    for (size_t i = 0; i < sampleCount; i++) {
        double left = sineAt(frequency, i);
        double right = left;
        effect.process(left, right);
        if (i >= skipCount) {
            sum += left * left;
            counted++;
        }
    }
    return 20.0 * std::log10(std::max(1.0e-9, std::sqrt(sum / static_cast<double>(counted)) / unitSineRms));
}

//! The response over the band the sweep lives in, one reading per log-spaced frequency.
std::vector<double> renderResponse(Phaser & effect, int points = 120)
{
    std::vector<double> response;
    for (int i = 0; i < points; i++) {
        const double frequency = 50.0 * std::pow(8000.0 / 50.0, static_cast<double>(i) / (points - 1));
        effect.reset();
        response.push_back(renderGainDb(effect, frequency));
    }
    return response;
}

//! Output RMS window by window, which is what a sweep looks like from the outside.
std::vector<double> renderRmsEnvelope(Phaser & effect, double frequency, double seconds)
{
    const auto sampleCount = static_cast<size_t>(testSampleRate * seconds);
    std::vector<double> envelope;
    double sum = 0.0;
    for (size_t i = 0; i < sampleCount; i++) {
        double left = sineAt(frequency, i);
        double right = left;
        effect.process(left, right);
        sum += left * left;
        if ((i + 1) % wholeCycleWindow == 0) {
            envelope.push_back(std::sqrt(sum / static_cast<double>(wholeCycleWindow)));
            sum = 0.0;
        }
    }
    return envelope;
}

//! How many times the sweep came back around, counted as upward crossings of the halfway level.
size_t countCycles(const std::vector<double> & envelope)
{
    const auto [min, max] = std::minmax_element(envelope.begin(), envelope.end());
    const double threshold = (*min + *max) / 2.0;
    size_t cycles = 0;
    for (size_t i = 1; i < envelope.size(); i++) {
        if (envelope[i - 1] <= threshold && envelope[i] > threshold) {
            cycles++;
        }
    }
    return cycles;
}

} // namespace

void PhaserTest::test_process_fullWet_shouldPassEverythingAtUnity()
{
    const auto effect = makeStaticPhaser(6, 1.0f);

    // An all-pass cascade only moves phase around: on its own it takes nothing out, at any
    // frequency. Everything the effect does audibly comes from summing that against the dry signal.
    for (auto && frequency : { 100.0, 440.0, 1000.0, 4000.0, 10000.0 }) {
        effect->reset();
        QVERIFY2(std::abs(renderGainDb(*effect, frequency)) < 0.5, qPrintable(QString { "At %1 Hz" }.arg(frequency)));
    }
}

void PhaserTest::test_process_mixedWithDry_shouldNotchTheSpectrum()
{
    const auto effect = makeStaticPhaser(6, 0.5f);
    const auto response = renderResponse(*effect);
    const auto [min, max] = std::minmax_element(response.begin(), response.end());

    // Cancellation where the cascade comes back half a cycle late, and next to nothing elsewhere.
    QVERIFY2(*min < -12.0, qPrintable(QString { "Deepest notch only %1 dB" }.arg(*min)));
    QVERIFY2(*max > -1.5, qPrintable(QString { "Nothing passes: best is %1 dB" }.arg(*max)));
}

void PhaserTest::test_process_moreStages_shouldNotchMoreDeeply()
{
    const auto notchCount = [](int stages) {
        const auto effect = makeStaticPhaser(stages, 0.5f);
        const auto response = renderResponse(*effect);
        size_t notches = 0;
        for (size_t i = 1; i + 1 < response.size(); i++) {
            if (response[i] < -6.0 && response[i] <= response[i - 1] && response[i] < response[i + 1]) {
                notches++;
            }
        }
        return notches;
    };

    // Every pair of all-pass sections puts one notch into the spectrum, so the stage count is
    // literally how many notches there are.
    QVERIFY(notchCount(12) > notchCount(4));
    QVERIFY(notchCount(4) >= 1);
}

void PhaserTest::test_depth_zero_shouldHoldTheNotchesStill()
{
    const auto effect = makeStaticPhaser(6, 0.5f);
    const auto envelope = renderRmsEnvelope(*effect, 1000.0, 1.0);
    const auto [min, max] = std::minmax_element(envelope.begin() + 10, envelope.end());

    QVERIFY(*max - *min < 0.001);
}

void PhaserTest::test_depth_full_shouldSweepTheNotches()
{
    Phaser effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), rateForHz(2.0));
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.5f);
    effect.sync();

    const auto envelope = renderRmsEnvelope(effect, 1000.0, 1.0);
    const auto [min, max] = std::minmax_element(envelope.begin(), envelope.end());

    // A notch passing over the tone takes it down and lets it back up. Note that a single tone dips
    // several times per sweep, not once: the cascade carries one notch per pair of stages, and the
    // sweep drags every one of them across the tone on the way up and again on the way down.
    QVERIFY(*max > *min * 2.0);
    QVERIFY(countCycles(envelope) > 0);
}

void PhaserTest::test_lfo_bpmMode_shouldFollowTempo()
{
    const auto cycles = [](float bpm) {
        Phaser effect;
        effect.setSampleRate(testSampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyLfoMode(), static_cast<float>(Lfo::Mode::BPM));
        setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), 0.25f); // A quarter note
        setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.5f);
        effect.sync();
        effect.setBpm(bpm);
        return countCycles(renderRmsEnvelope(effect, 1000.0, 2.0));
    };

    // Whatever the notches do to a single tone, twice the tempo has to do it twice as often.
    const auto slow = cycles(60.0f);
    const auto fast = cycles(120.0f);
    QVERIFY(slow > 0);
    QVERIFY2(std::abs(static_cast<int>(fast) - 2 * static_cast<int>(slow)) <= 1,
             qPrintable(QString { "%1 at 60 BPM but %2 at 120 BPM" }.arg(slow).arg(fast)));
}

void PhaserTest::test_rateDivider_shouldSlowTheSweep()
{
    const auto events = [](int divider) {
        Phaser effect;
        effect.setSampleRate(testSampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), rateForHz(1.0));
        setParameter(effect, Constants::NahdXml::xmlKeyRateDivider(), static_cast<float>(divider));
        setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.5f);
        effect.sync();
        // Slow enough that the readings resolve every notch passing over the tone: a faster sweep
        // moves more notches past it than the window rate can separate, and they merge.
        return countCycles(renderRmsEnvelope(effect, 1000.0, 4.0));
    };

    // The divider is the whole point of the control: it reaches rates the knob alone cannot.
    const auto undivided = events(1);
    QVERIFY(undivided > 0);
    QVERIFY2(std::abs(static_cast<int>(events(2)) * 2 - static_cast<int>(undivided)) <= 1,
             qPrintable(QString { "%1 undivided but %2 halved" }.arg(undivided).arg(events(2))));
    QVERIFY2(std::abs(static_cast<int>(events(4)) * 4 - static_cast<int>(undivided)) <= 2,
             qPrintable(QString { "%1 undivided but %2 quartered" }.arg(undivided).arg(events(4))));
    // Any whole number divides, not only the powers of two
    QVERIFY2(std::abs(static_cast<int>(events(3)) * 3 - static_cast<int>(undivided)) <= 2,
             qPrintable(QString { "%1 undivided but %2 at a third" }.arg(undivided).arg(events(3))));
}

void PhaserTest::test_rateDivider_bpmMode_shouldSlowTheSweep()
{
    const auto events = [](int divider) {
        Phaser effect;
        effect.setSampleRate(testSampleRate);
        setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
        setParameter(effect, Constants::NahdXml::xmlKeyLfoMode(), static_cast<float>(Lfo::Mode::BPM));
        setParameter(effect, Constants::NahdXml::xmlKeyLfoRate(), 0.25f); // A quarter note
        setParameter(effect, Constants::NahdXml::xmlKeyRateDivider(), static_cast<float>(divider));
        setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.5f);
        effect.sync();
        effect.setBpm(60.0f);
        return countCycles(renderRmsEnvelope(effect, 1000.0, 4.0));
    };

    // The divider sits after the tempo, so a locked sweep can be stretched over several bars, which
    // is a division the sync slider has no name for.
    const auto undivided = events(1);
    QVERIFY(undivided > 0);
    QVERIFY2(std::abs(static_cast<int>(events(4)) * 4 - static_cast<int>(undivided)) <= 2,
             qPrintable(QString { "%1 undivided but %2 quartered" }.arg(undivided).arg(events(4))));
}

void PhaserTest::test_feedback_full_shouldStayFinite()
{
    for (auto && feedback : { 0.0f, 1.0f }) {
        const auto effect = makeStaticPhaser(Phaser::maxStages(), 0.5f, feedback);
        double peak = 0.0;
        for (size_t i = 0; i < static_cast<size_t>(testSampleRate); i++) {
            double left = sineAt(1000.0, i);
            double right = left;
            effect->process(left, right);
            QVERIFY(std::isfinite(left));
            QVERIFY(std::isfinite(right));
            peak = std::max(peak, std::abs(left));
        }
        // An all-pass cascade passes everything at unity, so a feedback amount short of one rings
        // but cannot run away.
        QVERIFY2(peak < 20.0, qPrintable(QString { "Peak %1 at feedback %2" }.arg(peak).arg(feedback)));
    }
}

void PhaserTest::test_feedback_polarity_shouldChangeTheVoicing()
{
    const auto positive = makeStaticPhaser(6, 0.5f, 1.0f);
    const auto negative = makeStaticPhaser(6, 0.5f, 0.0f);

    const auto positiveResponse = renderResponse(*positive);
    const auto negativeResponse = renderResponse(*negative);

    // The two polarities cancel at different frequencies rather than differing only in amount.
    double largestDifference = 0.0;
    for (size_t i = 0; i < positiveResponse.size(); i++) {
        largestDifference = std::max(largestDifference, std::abs(positiveResponse[i] - negativeResponse[i]));
    }
    QVERIFY2(largestDifference > 6.0, qPrintable(QString { "Only %1 dB apart" }.arg(largestDifference)));
}

void PhaserTest::test_stereoPhase_quadrature_shouldOffsetChannels()
{
    Phaser effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.5f);
    effect.sync(); // Stereo phase defaults to a quarter cycle

    double maxDifference = 0.0;
    for (size_t i = 0; i < static_cast<size_t>(testSampleRate); i++) {
        double left = sineAt(1000.0, i);
        double right = left;
        effect.process(left, right);
        maxDifference = std::max(maxDifference, std::abs(left - right));
    }

    QVERIFY(maxDifference > 0.1);
}

void PhaserTest::test_stereoPhase_zero_shouldKeepChannelsIdentical()
{
    Phaser effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyStereoPhase(), 0.0f);
    effect.sync();

    for (size_t i = 0; i < static_cast<size_t>(testSampleRate); i++) {
        double left = sineAt(1000.0, i);
        double right = left;
        effect.process(left, right);
        QCOMPARE(left, right);
    }
}

void PhaserTest::test_reset_shouldRestartDeterministically()
{
    Phaser effect;
    effect.setSampleRate(testSampleRate);
    setParameter(effect, Constants::NahdXml::xmlKeyFeedback(), 0.9f);
    setParameter(effect, Constants::NahdXml::xmlKeyDepth(), 1.0f);
    effect.sync();

    const auto render = [&effect] {
        std::vector<double> output;
        for (size_t i = 0; i < 8192; i++) {
            double left = sineAt(1000.0, i);
            double right = left;
            effect.process(left, right);
            output.push_back(left);
        }
        return output;
    };

    effect.reset();
    const auto first = render();
    effect.reset();
    const auto second = render();

    // An offline render has to come out as the same audio as playback did.
    QCOMPARE(first, second);
}

void PhaserTest::test_mix_zero_shouldPassThroughDry()
{
    const auto effect = makeStaticPhaser(6, 0.0f);

    for (size_t i = 0; i < 1000; i++) {
        const double dry = sineAt(1000.0, i);
        double left = dry;
        double right = dry;
        effect->process(left, right);
        QCOMPARE(left, dry);
        QCOMPARE(right, dry);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PhaserTest)
