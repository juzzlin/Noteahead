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

#include "dynamic_eq_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/dynamic_eq.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr double Rate { 48000.0 };
namespace C = Constants::NahdXml;

void setParameter(DynamicEq & eq, const QString & key, float value)
{
    if (const auto p = eq.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    eq.sync();
}

void setRange(DynamicEq & eq, double db)
{
    setParameter(eq, C::xmlKeyRange(), static_cast<float>((db / DynamicEq::MaxRangeDb + 1.0) * 0.5));
}

void setFrequency(DynamicEq & eq, double hz)
{
    setParameter(eq, C::xmlKeyFrequency(), DynamicEq::frequencyPosition(hz));
}

//! A Dynamic EQ at 1 kHz with its defaults: -6 dB of Range from a -20 dB threshold, 4:1.
std::unique_ptr<DynamicEq> makeEq()
{
    auto eq = std::make_unique<DynamicEq>();
    eq->setSampleRate(Rate);
    return eq;
}

std::vector<double> sine(double hz, double amplitude, size_t frames)
{
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        stereo[i * 2] = stereo[i * 2 + 1] = amplitude * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(i) / Rate);
    }
    return stereo;
}

std::vector<double> render(DynamicEq & eq, std::vector<double> stereo, const std::vector<double> & external = {})
{
    const auto frames = static_cast<uint32_t>(stereo.size() / 2);
    AudioContext context { std::span(stereo.data(), stereo.size()), frames, static_cast<uint32_t>(Rate) };
    const std::array<std::span<const double>, 1> devices { std::span<const double> { external.data(), external.size() } };
    if (!external.empty()) {
        context.deviceOutputBuffers = std::span<const std::span<const double>> { devices.data(), devices.size() };
    }
    eq.processBlock(context);
    return stereo;
}

//! Level of the second half against the input's, in dB, once the band has settled.
double gainDb(const std::vector<double> & input, const std::vector<double> & output)
{
    double in = 0.0;
    double out = 0.0;
    for (size_t i = input.size() / 2; i < input.size(); i++) {
        in += input[i] * input[i];
        out += output[i] * output[i];
    }
    return 10.0 * std::log10(out / in);
}

} // namespace

void DynamicEqTest::test_processBlock_belowThreshold_shouldBeBitTransparent()
{
    // A band at 0 dB passes the signal through as it is, so a quiet signal is not touched at all
    auto eq = makeEq();
    const auto input = sine(1000.0, 0.01, 24000);
    QCOMPARE(render(*eq, input), input);
    QCOMPARE(eq->currentGainDb(), 0.0f);
}

void DynamicEqTest::test_processBlock_loudAtTheBand_shouldCutByRange()
{
    // -6 dBFS against a -20 dB threshold at 4:1 is 10.5 dB of reduction: the band goes the whole
    // 6 dB of its Range and stops there
    auto eq = makeEq();
    const auto input = sine(1000.0, 0.5, 48000);
    const auto output = render(*eq, input);
    QVERIFY2(std::abs(eq->currentGainDb() + 6.0f) < 0.05f, qPrintable(QString::number(eq->currentGainDb())));
    QVERIFY2(std::abs(gainDb(input, output) + 6.0) < 0.2, qPrintable(QString::number(gainDb(input, output))));
}

void DynamicEqTest::test_processBlock_positiveRange_shouldBoost()
{
    auto eq = makeEq();
    setRange(*eq, 6.0);
    const auto input = sine(1000.0, 0.4, 48000);
    const auto output = render(*eq, input);
    QVERIFY2(std::abs(gainDb(input, output) - 6.0) < 0.2, qPrintable(QString::number(gainDb(input, output))));
}

void DynamicEqTest::test_processBlock_rangeCap_shouldHoldEvenWhenFarOver()
{
    // Pushed far over the threshold, the band still never goes past its Range
    auto eq = makeEq();
    setRange(*eq, -3.0);
    setParameter(*eq, C::xmlKeyRatio(), 1.0f);
    render(*eq, sine(1000.0, 0.9, 48000));
    QVERIFY2(std::abs(eq->currentGainDb() + 3.0f) < 0.01f, qPrintable(QString::number(eq->currentGainDb())));
}

void DynamicEqTest::test_processBlock_bandDetector_shouldIgnoreOtherFrequencies()
{
    // Loud at 100 Hz, with the band and its detector at 5 kHz: nothing in the band's range to react to
    auto eq = makeEq();
    setFrequency(*eq, 5000.0);
    const auto input = sine(100.0, 0.5, 48000);
    const auto output = render(*eq, input);
    QVERIFY2(std::abs(eq->currentGainDb()) < 0.1f, qPrintable(QString::number(eq->currentGainDb())));
    QVERIFY(std::abs(gainDb(input, output)) < 0.05);
}

void DynamicEqTest::test_processBlock_wideDetector_shouldHearTheWholeSignal()
{
    // The same, but listening to everything: the 100 Hz tone moves the band at 5 kHz
    auto eq = makeEq();
    setFrequency(*eq, 5000.0);
    setParameter(*eq, C::xmlKeyDetectorFilter(), 1.0f);
    render(*eq, sine(100.0, 0.5, 48000));
    QVERIFY2(std::abs(eq->currentGainDb() + 6.0f) < 0.05f, qPrintable(QString::number(eq->currentGainDb())));
}

void DynamicEqTest::test_processBlock_flat_shouldMoveTheWholeSignal()
{
    // Flat moves the whole signal, whatever its frequency: a 100 Hz tone, detected wide
    auto eq = makeEq();
    setParameter(*eq, C::xmlKeyFilterType(), 3.0f);
    setParameter(*eq, C::xmlKeyDetectorFilter(), 1.0f);
    const auto input = sine(100.0, 0.5, 48000);
    const auto output = render(*eq, input);
    QVERIFY2(std::abs(gainDb(input, output) + 6.0) < 0.05, qPrintable(QString::number(gainDb(input, output))));
}

void DynamicEqTest::test_processBlock_detectorLpf_shouldIgnoreTheTop()
{
    // Detecting wide, a loud 8 kHz tone moves the band; with the detector low-passed it does not
    auto eq = makeEq();
    setParameter(*eq, C::xmlKeyDetectorFilter(), 1.0f);
    render(*eq, sine(8000.0, 0.5, 48000));
    QVERIFY(eq->currentGainDb() < -5.0f);

    auto filtered = makeEq();
    setParameter(*filtered, C::xmlKeyDetectorFilter(), 1.0f);
    setParameter(*filtered, C::xmlKeySideChainLpf(), 0.3f);
    render(*filtered, sine(8000.0, 0.5, 48000));
    QVERIFY2(std::abs(filtered->currentGainDb()) < 0.5f, qPrintable(QString::number(filtered->currentGainDb())));
}

void DynamicEqTest::test_processBlock_externalKey_shouldDriveTheBand()
{
    // A quiet signal, keyed by a loud one at the band: the key moves the band, the signal does not
    auto eq = makeEq();
    setParameter(*eq, C::xmlKeyDetectorSource(), 1.0f);
    setParameter(*eq, C::xmlKeySideChainSourceDevice(), 0.0f);
    QCOMPARE(eq->sidechainSourceDeviceIndex(), std::optional<size_t> { 0 });
    const auto input = sine(1000.0, 0.01, 48000);
    const auto output = render(*eq, input, sine(1000.0, 0.5, 48000));
    QVERIFY2(std::abs(eq->currentGainDb() + 6.0f) < 0.05f, qPrintable(QString::number(eq->currentGainDb())));
    QVERIFY2(std::abs(gainDb(input, output) + 6.0) < 0.2, qPrintable(QString::number(gainDb(input, output))));
}

void DynamicEqTest::test_processBlock_sideMode_shouldIgnoreTheMid()
{
    // All mid and loud: in Side mode the detector hears the silent side, and the mid is not shaped
    auto eq = makeEq();
    setParameter(*eq, C::xmlKeyStereoMode(), 2.0f);
    const auto input = sine(1000.0, 0.5, 24000);
    const auto output = render(*eq, input);
    QCOMPARE(eq->currentGainDb(), 0.0f);
    for (size_t i = 0; i < input.size(); i++) {
        QVERIFY(std::abs(output[i] - input[i]) < 1.0e-12);
    }
}

void DynamicEqTest::test_processBlock_release_shouldReturnToFlat()
{
    auto eq = makeEq();
    render(*eq, sine(1000.0, 0.5, 24000));
    QVERIFY(eq->currentGainDb() < -5.0f);
    QVERIFY(!eq->isSettled());
    // A second of silence is ten releases of 100 ms
    render(*eq, std::vector<double>(48000 * 2, 0.0));
    QVERIFY2(std::abs(eq->currentGainDb()) < 0.01f, qPrintable(QString::number(eq->currentGainDb())));
    QVERIFY(eq->isSettled());
}

void DynamicEqTest::test_processBlock_listen_shouldPlayTheDetector()
{
    // Listening wide is the input itself; through the band it is the band-passed input, which
    // takes most of a tone far from the band away
    auto wide = makeEq();
    setParameter(*wide, C::xmlKeyListen(), 1.0f);
    setParameter(*wide, C::xmlKeyDetectorFilter(), 1.0f);
    const auto input = sine(100.0, 0.01, 24000);
    QCOMPARE(render(*wide, input), input);

    auto band = makeEq();
    setParameter(*band, C::xmlKeyListen(), 1.0f);
    QVERIFY(gainDb(input, render(*band, input)) < -20.0);
}

void DynamicEqTest::test_gainHistory_shouldFollowTheBand()
{
    auto eq = makeEq();
    eq->setAnalysisEnabled(true);
    render(*eq, sine(1000.0, 0.5, 48000));
    const auto history = eq->gainHistory();
    QVERIFY(!history.empty());
    // Newest last: down at the Range by the end, and moving down on the way there
    QVERIFY2(std::abs(history.back() + 6.0f) < 0.1f, qPrintable(QString::number(history.back())));
    QVERIFY(std::ranges::is_sorted(history, std::greater<float> {}));
}

void DynamicEqTest::test_detectorLevelDb_shouldReadWhatTheDetectorHears()
{
    // Full range: the peak of the input, -6 dB for a sine at 0.5
    auto wide = makeEq();
    setParameter(*wide, C::xmlKeyDetectorFilter(), 1.0f);
    render(*wide, sine(1000.0, 0.5, 24000));
    QVERIFY2(std::abs(wide->detectorLevelDb() + 6.02f) < 0.2f, qPrintable(QString::number(wide->detectorLevelDb())));

    // Band only, at 5 kHz, of a 100 Hz tone: next to nothing
    auto band = makeEq();
    setFrequency(*band, 5000.0);
    render(*band, sine(100.0, 0.5, 24000));
    QVERIFY2(band->detectorLevelDb() < -40.0f, qPrintable(QString::number(band->detectorLevelDb())));

    // A side chain: the key's level, not this signal's
    auto keyed = makeEq();
    setParameter(*keyed, C::xmlKeyDetectorSource(), 1.0f);
    setParameter(*keyed, C::xmlKeyDetectorFilter(), 1.0f);
    setParameter(*keyed, C::xmlKeySideChainSourceDevice(), 0.0f);
    render(*keyed, sine(1000.0, 0.01, 24000), sine(1000.0, 0.25, 24000));
    QVERIFY2(std::abs(keyed->detectorLevelDb() + 12.04f) < 0.2f, qPrintable(QString::number(keyed->detectorLevelDb())));

    keyed->reset();
    QCOMPARE(keyed->detectorLevelDb(), -120.0f);
}

void DynamicEqTest::test_factoryPresets_shouldNameRealParameters()
{
    auto eq = makeEq();
    const auto & presets = eq->factoryPresets();
    QVERIFY(presets.size() > 1);
    QCOMPARE(presets.front().name, std::string { "Init" });
    for (auto && preset : presets) {
        for (auto && [key, value] : preset.parameters) {
            QVERIFY2(eq->parameter(key).has_value(), qPrintable(QString::fromStdString(preset.name + ": " + key)));
        }
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DynamicEqTest)
