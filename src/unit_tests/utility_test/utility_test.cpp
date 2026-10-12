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

#include "utility_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/utility.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double Rate { 48000.0 };

void setParameter(Utility & utility, const QString & key, float value)
{
    if (const auto p = utility.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    utility.sync();
}

std::unique_ptr<Utility> makeUtility()
{
    auto utility = std::make_unique<Utility>();
    utility->setSampleRate(Rate);
    return utility;
}

//! Runs @p stereo through, with @p reference as device 0's output when given.
std::vector<double> render(Utility & utility, std::vector<double> stereo, const std::vector<double> & reference = {})
{
    const auto frames = static_cast<uint32_t>(stereo.size() / 2);
    AudioContext context { std::span(stereo.data(), stereo.size()), frames, static_cast<uint32_t>(Rate) };
    const std::array<std::span<const double>, 1> devices { std::span<const double> { reference.data(), reference.size() } };
    if (!reference.empty()) {
        context.deviceOutputBuffers = std::span<const std::span<const double>> { devices.data(), devices.size() };
    }
    utility.processBlock(context);
    return stereo;
}

std::vector<double> noise(size_t frames, uint32_t seed)
{
    std::vector<double> stereo(frames * 2);
    std::mt19937 generator { seed };
    std::uniform_real_distribution<double> distribution { -0.5, 0.5 };
    std::ranges::generate(stereo, [&] { return distribution(generator); });
    return stereo;
}

//! Left and right in opposition: all side, at @p hz.
std::vector<double> sideSine(double hz, size_t frames)
{
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        const double sample = 0.5 * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(i) / Rate);
        stereo[i * 2] = sample;
        stereo[i * 2 + 1] = -sample;
    }
    return stereo;
}

double sideRms(const std::vector<double> & stereo, size_t fromFrame)
{
    double power = 0.0;
    size_t count = 0;
    for (size_t i = fromFrame; i < stereo.size() / 2; i++) {
        const double side = (stereo[i * 2] - stereo[i * 2 + 1]) * 0.5;
        power += side * side;
        count++;
    }
    return std::sqrt(power / static_cast<double>(count));
}

} // namespace

void UtilityTest::test_processBlock_defaults_shouldBeBitTransparent()
{
    // Dropped into a rack it does nothing until it is touched
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    const auto input = noise(4096, 1);
    QCOMPARE(render(utility, input), input);
}

void UtilityTest::test_processBlock_invertLeft_shouldFlipOnlyTheLeft()
{
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeyInvertLeft(), 1.0f);
    const auto input = noise(512, 2);
    const auto output = render(utility, input);
    for (size_t i = 0; i < 512; i++) {
        QCOMPARE(output[i * 2], -input[i * 2]);
        QCOMPARE(output[i * 2 + 1], input[i * 2 + 1]);
    }
}

void UtilityTest::test_processBlock_channelModes_shouldRouteTheChannels()
{
    const auto input = noise(256, 3);
    for (const int mode : { 1, 2, 3, 4 }) {
        const auto utilityOwner = makeUtility();
        auto & utility = *utilityOwner;
        setParameter(utility, Constants::NahdXml::xmlKeyChannelMode(), static_cast<float>(mode));
        const auto output = render(utility, input);
        for (size_t i = 0; i < 256; i++) {
            const double l = input[i * 2];
            const double r = input[i * 2 + 1];
            const auto [expectedL, expectedR] = mode == 1 ? std::pair { l, l } : mode == 2 ? std::pair { r, r }
              : mode == 3                                                                  ? std::pair { r, l }
                                                                                           : std::pair { (l + r) * 0.5, (l + r) * 0.5 };
            QCOMPARE(output[i * 2], expectedL);
            QCOMPARE(output[i * 2 + 1], expectedR);
        }
    }
}

void UtilityTest::test_processBlock_width_shouldScaleTheSide()
{
    const auto input = noise(256, 4);
    // None of the side is left at 0 %, which is mono
    const auto monoOwner = makeUtility();
    auto & mono = *monoOwner;
    setParameter(mono, Constants::NahdXml::xmlKeyWidth(), 0.0f);
    const auto folded = render(mono, input);
    for (size_t i = 0; i < 256; i++) {
        QVERIFY(std::abs(folded[i * 2] - folded[i * 2 + 1]) < 1.0e-12);
    }
    // Twice the side at 200 %, the mid unchanged
    const auto wideOwner = makeUtility();
    auto & wide = *wideOwner;
    setParameter(wide, Constants::NahdXml::xmlKeyWidth(), 1.0f);
    const auto widened = render(wide, input);
    for (size_t i = 0; i < 256; i++) {
        const double side = (input[i * 2] - input[i * 2 + 1]) * 0.5;
        const double mid = (input[i * 2] + input[i * 2 + 1]) * 0.5;
        QVERIFY(std::abs((widened[i * 2] - widened[i * 2 + 1]) * 0.5 - 2.0 * side) < 1.0e-12);
        QVERIFY(std::abs((widened[i * 2] + widened[i * 2 + 1]) * 0.5 - mid) < 1.0e-12);
    }
}

void UtilityTest::test_processBlock_monoBass_shouldFoldOnlyTheLowSide()
{
    // At the default 120 Hz: a 40 Hz side is all but gone, a 2 kHz one comes through whole
    const auto check = [](double hz) {
        const auto utilityOwner = makeUtility();
        auto & utility = *utilityOwner;
        setParameter(utility, Constants::NahdXml::xmlKeyMonoBass(), 1.0f);
        const auto input = sideSine(hz, 24000);
        return sideRms(render(utility, input), 12000) / sideRms(input, 12000);
    };
    QVERIFY2(check(40.0) < 0.05, qPrintable(QString::number(check(40.0))));
    QVERIFY2(std::abs(check(2000.0) - 1.0) < 0.01, qPrintable(QString::number(check(2000.0))));
}

void UtilityTest::test_processBlock_delay_shouldBeSampleExact()
{
    // 10 ms at 48 kHz is exactly 480 samples
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeyDelayTime(), 0.5f);
    QCOMPARE(Utility::delaySamples(0.5f, Rate), uint32_t { 480 });
    std::vector<double> impulse(2048 * 2, 0.0);
    impulse[0] = 1.0;
    impulse[1] = -1.0;
    const auto output = render(utility, impulse);
    for (size_t i = 0; i < 2048; i++) {
        QCOMPARE(output[i * 2], i == 480 ? 1.0 : 0.0);
        QCOMPARE(output[i * 2 + 1], i == 480 ? -1.0 : 0.0);
    }
}

void UtilityTest::test_latencyFrames_delay_shouldNotBeLatency()
{
    // The delay is the signal moving on purpose; a render that took it back out would undo it
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeyDelayTime(), 1.0f);
    QCOMPARE(utility.latencyFrames(Rate), size_t { 0 });
}

void UtilityTest::test_processBlock_gain_shouldTrim()
{
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    // -6 dB of +-24
    setParameter(utility, Constants::NahdXml::xmlKeyGain(), static_cast<float>((-6.0 / Utility::MaxGainDb + 1.0) * 0.5));
    const auto input = noise(256, 5);
    const auto output = render(utility, input);
    const double expected = std::pow(10.0, -6.0 / 20.0);
    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY(std::abs(output[i] - input[i] * expected) < 1.0e-12);
    }
}

void UtilityTest::test_processBlock_dcBlock_shouldRemoveAnOffset()
{
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeyDcBlock(), 1.0f);
    const auto output = render(utility, std::vector<double>(48000 * 2, 0.25));
    QVERIFY(std::abs(output[output.size() - 2]) < 0.01);
    QVERIFY(std::abs(output[output.size() - 1]) < 0.01);
}

void UtilityTest::test_reading_sameAsReference_shouldReinforce()
{
    // The same wave as the reference: as alike as it gets, and summing them adds 3 dB on
    // top of what two unrelated signals of their levels would make
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeySideChainSourceDevice(), 0.0f);
    utility.setAnalysisEnabled(true);
    const auto signal = noise(48000, 6);
    render(utility, signal, signal);
    const auto reading = utility.reading();
    QVERIFY(reading.hasReference);
    QVERIFY2(reading.correlation > 0.99f, qPrintable(QString::number(reading.correlation)));
    QVERIFY2(std::abs(reading.sumGainDb - 3.01f) < 0.1f, qPrintable(QString::number(reading.sumGainDb)));
}

void UtilityTest::test_reading_invertedReference_shouldCancel()
{
    // The reference upside down: opposed, and the two take each other out when summed
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeySideChainSourceDevice(), 0.0f);
    setParameter(utility, Constants::NahdXml::xmlKeyInvertLeft(), 1.0f);
    setParameter(utility, Constants::NahdXml::xmlKeyInvertRight(), 1.0f);
    utility.setAnalysisEnabled(true);
    const auto signal = noise(48000, 7);
    render(utility, signal, signal);
    const auto reading = utility.reading();
    QVERIFY2(reading.correlation < -0.99f, qPrintable(QString::number(reading.correlation)));
    QVERIFY2(reading.sumGainDb < -40.0f, qPrintable(QString::number(reading.sumGainDb)));
}

void UtilityTest::test_capture_hit_shouldHoldBothWaves()
{
    // A hit of the reference after silence is caught, with this signal over the same span
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeySideChainSourceDevice(), 0.0f);
    utility.setAnalysisEnabled(true);
    std::vector<double> reference(48000 * 2, 0.0);
    for (size_t i = 24000; i < 48000; i++) {
        reference[i * 2] = reference[i * 2 + 1] = 0.5 * std::sin(2.0 * std::numbers::pi * 55.0 * static_cast<double>(i - 24000) / Rate);
    }
    render(utility, reference, reference);
    render(utility, std::vector<double>(1024 * 2, 0.0), std::vector<double>(1024 * 2, 0.0));
    const auto capture = utility.capture();
    QVERIFY(!capture.reference.empty());
    QCOMPARE(capture.reference.size(), capture.signal.size());
    // The two waves are the same here, sample for sample
    for (size_t i = 0; i < capture.reference.size(); i++) {
        QCOMPARE(capture.signal[i], capture.reference[i]);
    }
    QVERIFY(std::ranges::any_of(capture.reference, [](float sample) { return std::abs(sample) > 0.1f; }));
}

void UtilityTest::test_isSettled_delay_shouldHoldTheTail()
{
    const auto utilityOwner = makeUtility();
    auto & utility = *utilityOwner;
    setParameter(utility, Constants::NahdXml::xmlKeyDelayTime(), 1.0f);
    render(utility, noise(256, 8));
    // Still holding 20 ms of it
    QVERIFY(!utility.isSettled());
    render(utility, std::vector<double>(2048 * 2, 0.0));
    QVERIFY(utility.isSettled());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::UtilityTest)
