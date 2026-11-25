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

#include "analog_bus_test.hpp"

#include "../../../../../common/constants.hpp"
#include "../../analog_bus.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

void setParameter(Effect & effect, const QString & key, float value)
{
    if (const auto p = effect.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    effect.sync();
}

float parameterValue(const Effect & effect, const QString & key)
{
    const auto p = effect.parameter(key.toStdString());
    return p ? p->get().value() : -1.0f;
}

double goertzelMagnitude(const std::vector<double> & signal, double frequencyHz, double sampleRate)
{
    const double w = 2.0 * std::numbers::pi * (frequencyHz / sampleRate);
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
    return std::sqrt(real * real + imag * imag) / (0.5 * static_cast<double>(signal.size()));
}

std::vector<double> renderSine(AnalogBus & bus, double frequencyHz, double amplitude, int totalFrames = 8192, int warmupFrames = 2048)
{
    bus.setSampleRate(SampleRate);
    std::vector<double> out;
    out.reserve(static_cast<size_t>(totalFrames - warmupFrames));
    for (int i = 0; i < totalFrames; i++) {
        double l = amplitude * std::sin(2.0 * std::numbers::pi * frequencyHz * static_cast<double>(i) / SampleRate);
        double r = l;
        bus.process(l, r);
        if (i >= warmupFrames) {
            out.push_back(l);
        }
    }
    return out;
}

double measureGainDb(AnalogBus & bus, double frequencyHz, double amplitude = 0.08)
{
    const auto out = renderSine(bus, frequencyHz, amplitude, 12000, 4096);
    const double mag = goertzelMagnitude(out, frequencyHz, SampleRate);
    return 20.0 * std::log10(std::max(1.0e-12, mag / amplitude));
}

double rmsDifference(const std::vector<double> & a, const std::vector<double> & b)
{
    double sumSq = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        const double d = a[i] - b[i];
        sumSq += d * d;
    }
    return std::sqrt(sumSq / static_cast<double>(a.size()));
}

} // namespace

void AnalogBusTest::test_parameterDefaultsAndConversions()
{
    AnalogBus bus;
    namespace C = Constants::NahdXml;

    QCOMPARE(bus.type(), Constants::RackEffectType::analogBus().toStdString());
    QCOMPARE(bus.typeId(), AnalogBus::typeIdString());
    QCOMPARE(parameterValue(bus, C::xmlKeyRouting()), static_cast<float>(AnalogBus::Routing::AToN));
    QCOMPARE(parameterValue(bus, C::xmlKeyColorMode()), static_cast<float>(AnalogBus::ColorMode::Off));
    QCOMPARE(parameterValue(bus, C::xmlKeyEqEnabled()), 1.0f);
    QCOMPARE(parameterValue(bus, C::xmlKeyEqPre()), 0.0f);
    QCOMPARE(parameterValue(bus, C::xmlKeyMix()), 1.0f);
    QCOMPARE(bus.saturationDb(), 0.0f);

    QVERIFY(std::abs(AnalogBus::driveDb(AnalogBus::drivePosition(18.0)) - 18.0) < 1.0e-4);
    QVERIFY(std::abs(AnalogBus::eqGainDb(AnalogBus::eqGainPosition(-4.5)) - (-4.5)) < 1.0e-4);
    QVERIFY(std::abs(AnalogBus::airGainDb(AnalogBus::airGainPosition(6.0)) - 6.0) < 1.0e-4);
    QVERIFY(std::abs(AnalogBus::outputGainDb(AnalogBus::outputGainPosition(-3.0)) - (-3.0)) < 1.0e-4);
}

void AnalogBusTest::test_tightHpf_attenuatesSubBassAndDifferentiatesModes()
{
    namespace C = Constants::NahdXml;

    AnalogBus offBus;
    setParameter(offBus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
    setParameter(offBus, C::xmlKeyLowCutMode(), static_cast<float>(AnalogBus::LowCutMode::Off));

    AnalogBus mixBus;
    setParameter(mixBus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
    setParameter(mixBus, C::xmlKeyLowCutMode(), static_cast<float>(AnalogBus::LowCutMode::Mix25Hz));

    AnalogBus trackBus;
    setParameter(trackBus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
    setParameter(trackBus, C::xmlKeyLowCutMode(), static_cast<float>(AnalogBus::LowCutMode::Track50Hz));

    const double offAt18 = measureGainDb(offBus, 18.0, 0.02);
    const double mixAt18 = measureGainDb(mixBus, 18.0, 0.02);
    const double trackAt18 = measureGainDb(trackBus, 18.0, 0.02);
    QVERIFY2(mixAt18 < offAt18 - 4.0, qPrintable(QString { "mixAt18=%1 offAt18=%2" }.arg(mixAt18).arg(offAt18)));
    QVERIFY2(trackAt18 < mixAt18 - 10.0, qPrintable(QString { "trackAt18=%1 mixAt18=%2" }.arg(trackAt18).arg(mixAt18)));

    // 25 Hz Mix mode gives a gentle resonant contour around 35 Hz while 50 Hz Track is still rolling down
    offBus.reset();
    mixBus.reset();
    trackBus.reset();
    const double offAt35 = measureGainDb(offBus, 35.0, 0.02);
    const double mixAt35 = measureGainDb(mixBus, 35.0, 0.02);
    const double trackAt35 = measureGainDb(trackBus, 35.0, 0.02);
    QVERIFY2(mixAt35 > offAt35 + 0.1, qPrintable(QString { "mixAt35=%1 offAt35=%2" }.arg(mixAt35).arg(offAt35)));
    QVERIFY2(trackAt35 < mixAt35 - 5.0, qPrintable(QString { "trackAt35=%1 mixAt35=%2" }.arg(trackAt35).arg(mixAt35)));
}

void AnalogBusTest::test_consoleRouting_stagesAndOrderingDifferentiateHarmonics()
{
    namespace C = Constants::NahdXml;

    const auto measureHarmonics = [&](AnalogBus::Routing routing) {
        AnalogBus bus;
        setParameter(bus, C::xmlKeyRouting(), static_cast<float>(routing));
        setParameter(bus, C::xmlKeyDriveA(), AnalogBus::drivePosition(22.0));
        setParameter(bus, C::xmlKeyDriveN(), AnalogBus::drivePosition(22.0));
        const auto out = renderSine(bus, 120.0, 0.15, 16000, 12000);
        const double h1 = goertzelMagnitude(out, 120.0, SampleRate);
        const double h2 = goertzelMagnitude(out, 240.0, SampleRate);
        const double h3 = goertzelMagnitude(out, 360.0, SampleRate);
        return std::tuple { out, h2 / h1, h3 / h1 };
    };

    const auto [outOff, offH2, offH3] = measureHarmonics(AnalogBus::Routing::Off);
    const auto [outA, aH2, aH3] = measureHarmonics(AnalogBus::Routing::A);
    const auto [outN, nH2, nH3] = measureHarmonics(AnalogBus::Routing::N);
    const auto [outAN, anH2, anH3] = measureHarmonics(AnalogBus::Routing::AToN);
    const auto [outNA, naH2, naH3] = measureHarmonics(AnalogBus::Routing::NToA);

    QVERIFY(offH2 < 1.0e-4 && offH3 < 1.0e-4);
    // Stage A is punchy and predominantly odd-harmonic; Stage N adds stronger even-harmonic warmth
    QVERIFY2(aH3 > 0.01 && nH2 > 0.01, qPrintable(QString { "aH3=%1 nH2=%2" }.arg(aH3).arg(nH2)));
    QVERIFY2(nH2 / nH3 > aH2 / aH3, qPrintable(QString { "N even/odd=%1, A even/odd=%2" }.arg(nH2 / nH3).arg(aH2 / aH3)));
    // Series order A -> N vs N -> A produces distinct nonlinear waveforms
    QVERIFY2(rmsDifference(outAN, outNA) > 1.0e-3, qPrintable(QString::number(rmsDifference(outAN, outNA))));
}

void AnalogBusTest::test_colorModules_produceDistinctColorationAndRespectColorMix()
{
    namespace C = Constants::NahdXml;

    const auto renderColor = [&](AnalogBus::ColorMode mode, float colorMix = 1.0f, double freq = 3500.0) {
        AnalogBus bus;
        setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
        setParameter(bus, C::xmlKeyColorMode(), static_cast<float>(mode));
        setParameter(bus, C::xmlKeyColorDrive(), 0.75f);
        setParameter(bus, C::xmlKeyColorMix(), colorMix);
        return renderSine(bus, freq, 0.15, 8192, 2048);
    };

    const auto clean = renderColor(AnalogBus::ColorMode::Off);
    const auto bus4k = renderColor(AnalogBus::ColorMode::Bus4000);
    const auto tape = renderColor(AnalogBus::ColorMode::Tape);
    const auto vinyl = renderColor(AnalogBus::ColorMode::Vinyl);
    const auto lofi = renderColor(AnalogBus::ColorMode::LoFi12);
    const auto tapeDry = renderColor(AnalogBus::ColorMode::Tape, 0.0f);

    QVERIFY(rmsDifference(clean, tapeDry) < 1.0e-12);
    QVERIFY(rmsDifference(clean, bus4k) > 5.0e-3);
    QVERIFY(rmsDifference(clean, tape) > 5.0e-3);
    QVERIFY(rmsDifference(clean, vinyl) > 5.0e-3);
    QVERIFY(rmsDifference(clean, lofi) > 5.0e-3);
    QVERIFY(rmsDifference(bus4k, tape) > 5.0e-3);
    QVERIFY(rmsDifference(tape, vinyl) > 5.0e-3);
    QVERIFY(rmsDifference(vinyl, lofi) > 5.0e-3);
}

void AnalogBusTest::test_toneEq_baxandallShelvesAirAndBypass()
{
    namespace C = Constants::NahdXml;

    AnalogBus bus;
    setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));

    // Low shelf boost lifts 60 Hz substantially more than 2 kHz
    setParameter(bus, C::xmlKeyLowFreq(), static_cast<float>(AnalogBus::LowFreq::Bass_65Hz));
    setParameter(bus, C::xmlKeyLowGain(), AnalogBus::eqGainPosition(9.0));
    const double lowBoost60 = measureGainDb(bus, 60.0);
    bus.reset();
    const double lowBoost2k = measureGainDb(bus, 2000.0);
    QVERIFY2(lowBoost60 > 4.5 && lowBoost2k < 0.5, qPrintable(QString { "60Hz=%1 2kHz=%2" }.arg(lowBoost60).arg(lowBoost2k)));

    // High shelf boost lifts 10 kHz substantially more than 200 Hz
    setParameter(bus, C::xmlKeyLowGain(), AnalogBus::eqGainPosition(0.0));
    setParameter(bus, C::xmlKeyHighFreq(), static_cast<float>(AnalogBus::HighFreq::Brite_8kHz));
    setParameter(bus, C::xmlKeyHighGain(), AnalogBus::eqGainPosition(9.0));
    bus.reset();
    const double highBoost10k = measureGainDb(bus, 10000.0);
    bus.reset();
    const double highBoost200 = measureGainDb(bus, 200.0);
    QVERIFY2(highBoost10k > 5.0 && highBoost200 < 0.5, qPrintable(QString { "10kHz=%1 200Hz=%2" }.arg(highBoost10k).arg(highBoost200)));

    // Air shelf +9 dB lifts 12 kHz cleanly
    setParameter(bus, C::xmlKeyHighGain(), AnalogBus::eqGainPosition(0.0));
    setParameter(bus, C::xmlKeyAirGain(), AnalogBus::airGainPosition(9.0));
    bus.reset();
    const double airBoost12k = measureGainDb(bus, 12000.0);
    QVERIFY2(airBoost12k > 2.5, qPrintable(QString { "airBoost12k=%1" }.arg(airBoost12k)));

    // Disabling EQ bypasses all boosts cleanly
    setParameter(bus, C::xmlKeyLowGain(), AnalogBus::eqGainPosition(9.0));
    setParameter(bus, C::xmlKeyEqEnabled(), 0.0f);
    bus.reset();
    const double bypassed60 = measureGainDb(bus, 60.0);
    QVERIFY2(std::abs(bypassed60) < 0.05, qPrintable(QString { "bypassed60=%1" }.arg(bypassed60)));
}

void AnalogBusTest::test_toneEq_preVsPostAffectsSaturation()
{
    namespace C = Constants::NahdXml;

    auto runEqPos = [](bool pre) {
        AnalogBus bus;
        setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::A));
        setParameter(bus, C::xmlKeyDriveA(), AnalogBus::drivePosition(20.0));
        setParameter(bus, C::xmlKeyLowFreq(), static_cast<float>(AnalogBus::LowFreq::Body_110Hz));
        setParameter(bus, C::xmlKeyLowGain(), AnalogBus::eqGainPosition(11.0));
        setParameter(bus, C::xmlKeyEqPre(), pre ? 1.0f : 0.0f);
        const auto out = renderSine(bus, 80.0, 0.12, 8192, 2048);
        return std::pair { out, bus.saturationDb() };
    };

    const auto [preOut, preSat] = runEqPos(true);
    const auto [postOut, postSat] = runEqPos(false);
    // Driving +11 dB into Stage A hits saturation significantly harder than boosting after Stage A
    QVERIFY2(preSat < postSat - 1.5f, qPrintable(QString { "preSat=%1 postSat=%2" }.arg(preSat).arg(postSat)));
    QVERIFY(rmsDifference(preOut, postOut) > 0.05);
}

void AnalogBusTest::test_vintage_rollsOffHighFrequencies()
{
    namespace C = Constants::NahdXml;

    AnalogBus modern;
    setParameter(modern, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
    setParameter(modern, C::xmlKeyVintage(), 0.0f);

    AnalogBus vintage;
    setParameter(vintage, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
    setParameter(vintage, C::xmlKeyVintage(), 1.0f);

    const double modern15k = measureGainDb(modern, 15000.0, 0.05);
    const double vintage15k = measureGainDb(vintage, 15000.0, 0.05);
    modern.reset();
    vintage.reset();
    const double vintage200 = measureGainDb(vintage, 200.0, 0.05);

    QVERIFY2(vintage15k < modern15k - 2.0, qPrintable(QString { "vintage15k=%1 modern15k=%2" }.arg(vintage15k).arg(modern15k)));
    QVERIFY2(std::abs(vintage200) < 0.1, qPrintable(QString { "vintage200=%1" }.arg(vintage200)));
}

void AnalogBusTest::test_aspect_widensStereoAndPreservesMono()
{
    namespace C = Constants::NahdXml;

    // Mono signal must remain strictly mono even at 100% Aspect
    AnalogBus monoBus;
    setParameter(monoBus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
    setParameter(monoBus, C::xmlKeyAspect(), 1.0f);
    monoBus.setSampleRate(SampleRate);
    double maxDiff = 0.0;
    for (int i = 0; i < 4096; i++) {
        double l = 0.2 * std::sin(2.0 * std::numbers::pi * 2500.0 * static_cast<double>(i) / SampleRate);
        double r = l;
        monoBus.process(l, r);
        maxDiff = std::max(maxDiff, std::abs(l - r));
    }
    QVERIFY2(maxDiff < 1.0e-12, qPrintable(QString::number(maxDiff)));

    // Stereo signal gets widened at highs while sub-bass side stays tight
    const auto measureSideRms = [](float aspect, double freqHz) {
        AnalogBus bus;
        setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::Off));
        setParameter(bus, C::xmlKeyAspect(), aspect);
        bus.setSampleRate(SampleRate);
        double sumSq = 0.0;
        const int count = 4096;
        for (int i = 0; i < 6144; i++) {
            double l = 0.2 * std::sin(2.0 * std::numbers::pi * freqHz * static_cast<double>(i) / SampleRate);
            double r = 0.2 * std::cos(2.0 * std::numbers::pi * freqHz * static_cast<double>(i) / SampleRate);
            bus.process(l, r);
            if (i >= 2048) {
                const double side = 0.5 * (l - r);
                sumSq += side * side;
            }
        }
        return std::sqrt(sumSq / count);
    };

    const double baseHighSide = measureSideRms(0.0f, 3000.0);
    const double wideHighSide = measureSideRms(1.0f, 3000.0);
    const double baseLowSide = measureSideRms(0.0f, 55.0);
    const double wideLowSide = measureSideRms(1.0f, 55.0);

    QVERIFY2(wideHighSide / baseHighSide > 1.8, qPrintable(QString { "high ratio=%1" }.arg(wideHighSide / baseHighSide)));
    QVERIFY2(wideLowSide / baseLowSide < 1.3, qPrintable(QString { "low ratio=%1" }.arg(wideLowSide / baseLowSide)));
}

void AnalogBusTest::test_analog_introducesDeterministicLrDifference()
{
    namespace C = Constants::NahdXml;

    auto renderStereo = [](bool analogOn) {
        AnalogBus bus;
        setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::A));
        setParameter(bus, C::xmlKeyDriveA(), AnalogBus::drivePosition(18.0));
        setParameter(bus, C::xmlKeyAnalog(), analogOn ? 1.0f : 0.0f);
        bus.setSampleRate(SampleRate);
        double diffSumSq = 0.0;
        std::vector<double> lefts;
        for (int i = 0; i < 4096; i++) {
            double l = 0.15 * std::sin(2.0 * std::numbers::pi * 440.0 * static_cast<double>(i) / SampleRate);
            double r = l;
            bus.process(l, r);
            const double d = l - r;
            diffSumSq += d * d;
            lefts.push_back(l);
        }
        return std::pair { std::sqrt(diffSumSq / 4096.0), lefts };
    };

    const auto [cleanDiff, cleanL] = renderStereo(false);
    const auto [analogDiff1, analogL1] = renderStereo(true);
    const auto [analogDiff2, analogL2] = renderStereo(true);

    QVERIFY(cleanDiff < 1.0e-12);
    QVERIFY2(analogDiff1 > 1.0e-4 && analogDiff1 < 0.02, qPrintable(QString::number(analogDiff1)));
    QVERIFY(rmsDifference(analogL1, analogL2) < 1.0e-12);
}

void AnalogBusTest::test_oversampling_mixSendModeAndMetering()
{
    namespace C = Constants::NahdXml;

    // 1. Oversampling reduces aliasing above Nyquist when driving hard
    auto measureAlias = [](uint8_t factor) {
        AnalogBus bus;
        setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::AToN));
        setParameter(bus, C::xmlKeyDriveA(), AnalogBus::drivePosition(30.0));
        setParameter(bus, C::xmlKeyDriveN(), AnalogBus::drivePosition(24.0));
        bus.setOversampleFactor(factor);
        const auto out = renderSine(bus, 10000.0, 0.25, 4096, 512);
        // 3rd harmonic of 10 kHz at 48 kHz folds back to 18 kHz
        return std::pair { goertzelMagnitude(out, 18000.0, SampleRate), bus.saturationDb() };
    };

    const auto [alias1x, sat1x] = measureAlias(1);
    const auto [alias4x, sat4x] = measureAlias(4);
    QVERIFY2(alias4x < alias1x * 0.25, qPrintable(QString { "alias1x=%1 alias4x=%2" }.arg(alias1x).arg(alias4x)));
    QVERIFY2(sat4x < -2.0f, qPrintable(QString { "sat4x=%1" }.arg(sat4x)));

    // 2. Mix = 0 passes raw input unmodified at factor = 1; SendMode returns dry + mix * wet
    AnalogBus bus;
    bus.setSampleRate(SampleRate);
    setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::A));
    setParameter(bus, C::xmlKeyMix(), 0.0f);
    double l = 0.25;
    double r = -0.15;
    bus.process(l, r);
    QCOMPARE(l, 0.25);
    QCOMPARE(r, -0.15);

    bus.reset();
    QCOMPARE(bus.saturationDb(), 0.0f);

    bus.setSendMode(true);
    setParameter(bus, C::xmlKeyMix(), 0.0f);
    l = 0.25;
    r = -0.15;
    bus.process(l, r);
    QCOMPARE(l, 0.25);
    QCOMPARE(r, -0.15);

    setParameter(bus, C::xmlKeyMix(), 0.5f);
    double sl = 0.25;
    double sr = -0.15;
    bus.process(sl, sr);
    QVERIFY(std::abs(sl - 0.25) > 0.05);
}

void AnalogBusTest::test_presets_loadAndRenderCleanly()
{
    AnalogBus bus;
    bus.setSampleRate(SampleRate);
    const auto & presets = bus.factoryPresets();
    QVERIFY(presets.size() >= 8);
    QCOMPARE(presets.front().name, std::string { "Init" });

    for (size_t idx = 0; idx < presets.size(); idx++) {
        QVERIFY(bus.applyFactoryPreset(idx));
        bus.reset();
        for (int i = 0; i < 256; i++) {
            double l = 0.1 * std::sin(2.0 * std::numbers::pi * 330.0 * static_cast<double>(i) / SampleRate);
            double r = 0.1 * std::cos(2.0 * std::numbers::pi * 330.0 * static_cast<double>(i) / SampleRate);
            bus.process(l, r);
            QVERIFY(std::isfinite(l) && std::isfinite(r));
        }
    }
}

void AnalogBusTest::test_reset_restoresExactReplay()
{
    namespace C = Constants::NahdXml;

    AnalogBus bus;
    setParameter(bus, C::xmlKeyRouting(), static_cast<float>(AnalogBus::Routing::AToN));
    setParameter(bus, C::xmlKeyColorMode(), static_cast<float>(AnalogBus::ColorMode::LoFi12));
    setParameter(bus, C::xmlKeyLowGain(), AnalogBus::eqGainPosition(3.5));
    setParameter(bus, C::xmlKeyAnalog(), 1.0f);
    setParameter(bus, C::xmlKeyAspect(), 0.5f);
    bus.setSampleRate(SampleRate);

    auto runBlock = [&]() {
        std::vector<double> out;
        out.reserve(512);
        for (int i = 0; i < 256; i++) {
            double l = 0.12 * std::sin(2.0 * std::numbers::pi * 440.0 * static_cast<double>(i) / SampleRate);
            double r = 0.10 * std::cos(2.0 * std::numbers::pi * 660.0 * static_cast<double>(i) / SampleRate);
            bus.process(l, r);
            out.push_back(l);
            out.push_back(r);
        }
        return out;
    };

    const auto first = runBlock();
    bus.reset();
    const auto second = runBlock();
    QVERIFY(rmsDifference(first, second) < 1.0e-12);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AnalogBusTest)
