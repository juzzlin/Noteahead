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

#include "eq_32_band_graphical_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/dsp/graphic_eq_designer.hpp"
#include "../../domain/dsp/svf_filter.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/eq_32_band_graphical.hpp"

#include <QTest>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <span>
#include <vector>

namespace noteahead {

namespace {

using Gains = GraphicEqDesigner::Gains;

void setParameter(Eq32BandGraphical & eq, const QString & key, float value)
{
    if (const auto p = eq.parameter(key.toStdString()); p) {
        p->get().update(value);
    }
    eq.sync();
}

void setSliders(Eq32BandGraphical & eq, const Gains & gainsDb)
{
    for (size_t i = 0; i < gainsDb.size(); i++) {
        setParameter(eq, Constants::NahdXml::xmlKeyBandGain(i), Eq32BandGraphical::sliderParameterValue(gainsDb.at(i)));
    }
}

//! Slider settings that are hard in different ways: every other band against its neighbours, a
//! smooth sweep, random, and the top four up together, where the bands crowd Nyquist.
std::vector<Gains> testSettings()
{
    std::vector<Gains> settings(4);
    std::mt19937 generator { 1 };
    std::uniform_real_distribution<double> random { -12.0, 12.0 };
    for (size_t i = 0; i < Eq32BandGraphical::BandCount; i++) {
        settings.at(0).at(i) = i % 2 ? 12.0 : -12.0;
        settings.at(1).at(i) = 12.0 * std::sin(static_cast<double>(i) / 4.0);
        settings.at(2).at(i) = random(generator);
        settings.at(3).at(i) = i >= 28 ? 12.0 : 0.0;
    }
    return settings;
}

std::vector<double> render(Eq32BandGraphical & eq, std::vector<double> stereo)
{
    const auto frames = static_cast<uint32_t>(stereo.size() / 2);
    AudioContext context { std::span(stereo.data(), stereo.size()), frames, static_cast<uint32_t>(eq.sampleRate()) };
    eq.processBlock(context);
    return stereo;
}

//! Gain in dB the equalizer gives a sine at @p frequency in both channels, measured after it settles.
double measuredGainDb(Eq32BandGraphical & eq, double frequency)
{
    const double rate = eq.sampleRate();
    // Two seconds, measured over the second: a linear-phase kernel is over half a second long and
    // only settles once it is full
    const auto frames = static_cast<size_t>(rate * 2.0);
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        stereo[i * 2] = stereo[i * 2 + 1] = std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / rate);
    }
    eq.reset();
    const auto output = render(eq, stereo);
    double in = 0.0;
    double out = 0.0;
    for (size_t i = frames / 2; i < frames; i++) {
        in += stereo[i * 2] * stereo[i * 2];
        out += output[i * 2] * output[i * 2];
    }
    return 10.0 * std::log10(out / in);
}

//! Everything under 1 kHz up by 8 dB, the rest flat: broad enough to make a mix much louder.
Gains lowBoost()
{
    Gains sliders {};
    for (size_t i = 0; i < Eq32BandGraphical::BandCount; i++) {
        sliders.at(i) = GraphicEqDesigner::centreFrequency(i) < 1000.0 ? 8.0 : 0.0;
    }
    return sliders;
}

//! Output over input level, in dB, of four seconds of pink noise through @p eq. The first second
//! is left out, for the filters, the linear-phase kernel and the gain ramp to settle. The noise is
//! cut below 20 Hz, where auto gain does not look: the generator puts another octave there, and a
//! low shelf would lift it.
double pinkNoiseGainDb(Eq32BandGraphical & eq)
{
    std::mt19937 generator { 1234 };
    std::uniform_real_distribution<double> white { -1.0, 1.0 };
    // Paul Kellet's refined filter: within a fraction of a decibel of -3 dB/octave over the band
    std::array<double, 7> b {};
    // A 24 dB/oct Butterworth
    SvfFilter lowCut1;
    SvfFilter lowCut2;
    lowCut1.calculateLowCut(20.0, eq.sampleRate(), 0.5412);
    lowCut2.calculateLowCut(20.0, eq.sampleRate(), 1.3066);
    const auto frames = static_cast<size_t>(eq.sampleRate() * 4.0);
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        const double w = white(generator);
        b[0] = 0.99886 * b[0] + w * 0.0555179;
        b[1] = 0.99332 * b[1] + w * 0.0750759;
        b[2] = 0.96900 * b[2] + w * 0.1538520;
        b[3] = 0.86650 * b[3] + w * 0.3104856;
        b[4] = 0.55000 * b[4] + w * 0.5329522;
        b[5] = -0.7616 * b[5] - w * 0.0168980;
        stereo[i * 2] = stereo[i * 2 + 1] = lowCut2.process(lowCut1.process((b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362) * 0.1));
        b[6] = w * 0.115926;
    }
    eq.reset();
    const auto output = render(eq, stereo);
    double in = 0.0;
    double out = 0.0;
    for (size_t i = frames / 4; i < frames; i++) {
        in += stereo[i * 2] * stereo[i * 2];
        out += output[i * 2] * output[i * 2];
    }
    return 10.0 * std::log10(out / in);
}

std::unique_ptr<Eq32BandGraphical> linearPhaseEq(const Gains & sliders, int stereoMode = 0)
{
    auto eq = std::make_unique<Eq32BandGraphical>();
    eq->setSampleRate(48000.0);
    setSliders(*eq, sliders);
    setParameter(*eq, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(stereoMode));
    setParameter(*eq, Constants::NahdXml::xmlKeyLinearPhase(), 1.0f);
    return eq;
}

//! The response to an impulse @p warmUp frames into the run, from the impulse on.
std::vector<double> impulseResponse(Eq32BandGraphical & eq, size_t frames, size_t warmUp = 0)
{
    if (warmUp > 0) {
        render(eq, std::vector<double>(warmUp * 2, 0.0));
    }
    std::vector<double> stereo(frames * 2, 0.0);
    stereo[0] = stereo[1] = 1.0;
    const auto output = render(eq, stereo);
    std::vector<double> left(frames);
    for (size_t i = 0; i < frames; i++) {
        left[i] = output[i * 2];
    }
    return left;
}

} // namespace

void Eq32BandGraphicalTest::test_magnitudeDbAt_centres_shouldMatchTheSliders()
{
    // What a graphic equalizer promises: the curve goes through the sliders
    for (const double rate : { 44100.0, 48000.0, 96000.0 }) {
        for (const auto & sliders : testSettings()) {
            Eq32BandGraphical eq;
            eq.setSampleRate(rate);
            setSliders(eq, sliders);
            std::vector<double> centres;
            for (size_t i = 0; i < Eq32BandGraphical::BandCount; i++) {
                centres.push_back(GraphicEqDesigner::centreFrequency(i));
            }
            const auto response = eq.magnitudesDbAt(centres);
            for (size_t i = 0; i < Eq32BandGraphical::BandCount; i++) {
                QVERIFY2(std::abs(response.at(i) - sliders.at(i)) < 0.15, qPrintable(QString { "rate %1, band %2: %3 dB for %4" }.arg(rate).arg(i).arg(response.at(i)).arg(sliders.at(i))));
            }
        }
    }
}

void Eq32BandGraphicalTest::test_magnitudeDbAt_allUp_shouldStayFlatToTheTop()
{
    // Every slider up is a level change and nothing else. The top octave is the test: bilinear bells
    // pinched there and left a dip of several decibels between 16 and 20 kHz.
    for (const double rate : { 44100.0, 48000.0 }) {
        for (const double gain : { 12.0, -12.0 }) {
            Eq32BandGraphical eq;
            eq.setSampleRate(rate);
            Gains sliders;
            sliders.fill(gain);
            setSliders(eq, sliders);
            std::vector<double> frequencies;
            for (double hz = 16.0; hz <= 20000.0; hz *= 1.01) {
                frequencies.push_back(hz);
            }
            const auto response = eq.magnitudesDbAt(frequencies);
            for (size_t i = 0; i < frequencies.size(); i++) {
                QVERIFY2(std::abs(response.at(i) - gain) < 0.6, qPrintable(QString { "rate %1, %2 Hz: %3 dB" }.arg(rate).arg(frequencies.at(i)).arg(response.at(i))));
            }
        }
    }
}

void Eq32BandGraphicalTest::test_processBlock_flat_shouldBeBitTransparent()
{
    for (const int mode : { 0, 1, 2 }) {
        Eq32BandGraphical eq;
        eq.setSampleRate(48000.0);
        setParameter(eq, Constants::NahdXml::xmlKeyStereoMode(), static_cast<float>(mode));
        std::vector<double> stereo(2048);
        std::mt19937 generator { 2 };
        std::uniform_real_distribution<double> noise { -1.0, 1.0 };
        std::ranges::generate(stereo, [&] { return noise(generator); });

        const auto output = render(eq, stereo);
        if (mode == 0) {
            QCOMPARE(output, stereo);
        } else {
            // Mid and Side go through a sum and a difference, which round
            for (size_t i = 0; i < stereo.size(); i++) {
                QVERIFY(std::abs(output.at(i) - stereo.at(i)) < 1.0e-15);
            }
        }
    }
}

void Eq32BandGraphicalTest::test_processBlock_sine_shouldMatchTheCurve()
{
    // The audio and the drawn curve come from the same bands, so what is heard is what is shown
    Eq32BandGraphical eq;
    eq.setSampleRate(48000.0);
    setSliders(eq, testSettings().at(1));
    for (const double hz : { 50.0, 400.0, 1000.0, 3150.0, 12500.0 }) {
        const double measured = measuredGainDb(eq, hz);
        QVERIFY2(std::abs(measured - eq.magnitudeDbAt(hz)) < 0.1, qPrintable(QString { "%1 Hz: %2 dB measured, %3 dB drawn" }.arg(hz).arg(measured).arg(eq.magnitudeDbAt(hz))));
    }
}

void Eq32BandGraphicalTest::test_processBlock_midMode_shouldLeaveTheSideAlone()
{
    Eq32BandGraphical eq;
    eq.setSampleRate(48000.0);
    setSliders(eq, testSettings().at(2));
    setParameter(eq, Constants::NahdXml::xmlKeyStereoMode(), 1.0f);
    // Left and right in opposition: all side, no mid
    std::vector<double> stereo(4096);
    for (size_t i = 0; i < stereo.size() / 2; i++) {
        const double sample = std::sin(static_cast<double>(i) * 0.05);
        stereo[i * 2] = sample;
        stereo[i * 2 + 1] = -sample;
    }
    QCOMPARE(render(eq, stereo), stereo);
}

void Eq32BandGraphicalTest::test_processBlock_sideMode_shouldLeaveTheMidAlone()
{
    Eq32BandGraphical eq;
    eq.setSampleRate(48000.0);
    setSliders(eq, testSettings().at(2));
    setParameter(eq, Constants::NahdXml::xmlKeyStereoMode(), 2.0f);
    std::vector<double> stereo(4096);
    for (size_t i = 0; i < stereo.size() / 2; i++) {
        stereo[i * 2] = stereo[i * 2 + 1] = std::sin(static_cast<double>(i) * 0.05);
    }
    QCOMPARE(render(eq, stereo), stereo);
}

void Eq32BandGraphicalTest::test_processBlock_zigzag_shouldStayStable()
{
    // The hardest setting asks the bands for nearly 30 dB each; noise through it has to stay bounded
    for (const double rate : { 44100.0, 96000.0, 192000.0 }) {
        Eq32BandGraphical eq;
        eq.setSampleRate(rate);
        setSliders(eq, testSettings().at(0));
        std::vector<double> stereo(static_cast<size_t>(rate) * 2);
        std::mt19937 generator { 3 };
        std::uniform_real_distribution<double> noise { -0.5, 0.5 };
        std::ranges::generate(stereo, [&] { return noise(generator); });
        const auto output = render(eq, stereo);
        QVERIFY(std::ranges::all_of(output, [](double sample) { return std::isfinite(sample) && std::abs(sample) < 100.0; }));
    }
}

void Eq32BandGraphicalTest::test_processBlock_lowSampleRate_shouldStayFinite()
{
    // At 22.05 kHz the top bands are above Nyquist and are passed through rather than designed
    Eq32BandGraphical eq;
    eq.setSampleRate(22050.0);
    Gains sliders;
    sliders.fill(6.0);
    setSliders(eq, sliders);
    QVERIFY(!GraphicEqDesigner::isBandUsable(31, 22050.0));
    std::vector<double> stereo(4096, 0.25);
    const auto output = render(eq, stereo);
    QVERIFY(std::ranges::all_of(output, [](double sample) { return std::isfinite(sample); }));
    QVERIFY(std::abs(eq.magnitudeDbAt(1000.0) - 6.0) < 0.15);
}

void Eq32BandGraphicalTest::test_outputGain_shouldScaleTheCurve()
{
    Eq32BandGraphical eq;
    eq.setSampleRate(48000.0);
    setParameter(eq, Constants::NahdXml::xmlKeyGain(), Eq32BandGraphical::sliderParameterValue(-6.0));
    QVERIFY(std::abs(eq.magnitudeDbAt(1000.0) + 6.0) < 1.0e-9);
    QVERIFY(std::abs(measuredGainDb(eq, 1000.0) + 6.0) < 0.01);
}

void Eq32BandGraphicalTest::test_factoryPresets_flat_shouldComeFirst()
{
    Eq32BandGraphical eq;
    QVERIFY(eq.factoryPresets().size() > 1);
    QCOMPARE(eq.factoryPresets().front().name, std::string { "Flat" });
    QVERIFY(eq.factoryPresets().front().parameters.empty());
}

void Eq32BandGraphicalTest::test_factoryPresets_slider_shouldLandOnItsBand()
{
    // The presets are written by nominal frequency; 315 Hz is band 13, and De-Mud takes 3 dB there
    Eq32BandGraphical eq;
    eq.setSampleRate(48000.0);
    const auto & presets = eq.factoryPresets();
    const auto deMud = std::ranges::find_if(presets, [](auto && preset) { return preset.name == "De-Mud"; });
    QVERIFY(deMud != presets.end());
    QVERIFY(eq.applyFactoryPreset(static_cast<size_t>(deMud - presets.begin())));
    eq.sync();
    QCOMPARE(Eq32BandGraphical::nominalLabel(13), std::string { "315 Hz" });
    QVERIFY(std::abs(eq.magnitudeDbAt(GraphicEqDesigner::centreFrequency(13)) + 3.0) < 0.15);
}

void Eq32BandGraphicalTest::test_parameterDisplay_everyBand_shouldBeDescribed()
{
    // Described parameters are what parameter automation offers, in their units
    Eq32BandGraphical eq;
    for (size_t i = 0; i < Eq32BandGraphical::BandCount; i++) {
        const auto parameter = eq.parameter(Constants::NahdXml::xmlKeyBandGain(i).toStdString());
        QVERIFY(parameter.has_value());
        const auto & display = parameter->get().display();
        QVERIFY(display.has_value());
        QCOMPARE(display->unit, std::string { "dB" });
        QCOMPARE(display->label, Eq32BandGraphical::nominalLabel(i));
    }
}

void Eq32BandGraphicalTest::test_linearPhase_off_shouldHaveNoLatency()
{
    Eq32BandGraphical eq;
    QCOMPARE(eq.latencyFrames(48000.0), size_t { 0 });
    // 32768 taps at 48 kHz: half of them, and one of the 32 partitions
    QCOMPARE(Eq32BandGraphical::linearPhaseLatencyFrames(48000.0), size_t { 16384 + 1024 });
    QCOMPARE(Eq32BandGraphical::linearPhaseLatencyFrames(96000.0), size_t { 32768 + 2048 });
}

void Eq32BandGraphicalTest::test_linearPhase_impulse_shouldArriveAtTheLatency()
{
    // Flat: the kernel is a pure delay, so the impulse comes out whole, exactly that late
    auto eq = linearPhaseEq({});
    const auto latency = eq->latencyFrames(48000.0);
    QCOMPARE(latency, Eq32BandGraphical::linearPhaseLatencyFrames(48000.0));
    const auto response = impulseResponse(*eq, latency * 2);
    for (size_t i = 0; i < response.size(); i++) {
        QVERIFY2(std::abs(response.at(i) - (i == latency ? 1.0 : 0.0)) < 1.0e-9, qPrintable(QString { "frame %1: %2" }.arg(i).arg(response.at(i))));
    }
}

void Eq32BandGraphicalTest::test_linearPhase_impulseResponse_shouldBeSymmetric()
{
    // Linear phase is a symmetric impulse response: the same before its centre as after
    auto eq = linearPhaseEq(testSettings().at(2));
    const auto latency = eq->latencyFrames(48000.0);
    // Once the kernel is built; see the cold start below for the first moments
    const auto response = impulseResponse(*eq, latency * 2, latency);
    for (size_t n = 1; n < 16000; n++) {
        QVERIFY2(std::abs(response.at(latency + n) - response.at(latency - n)) < 1.0e-9, qPrintable(QString::number(n)));
    }
}

void Eq32BandGraphicalTest::test_linearPhase_coldStart_shouldBeReadyWithinTheLatency()
{
    // The kernel is built in slices even from cold, and has to be in place well before anything
    // reaches the output. From then on the output is the finished filter's exactly: only the
    // pre-ringing of the first few thousand frames is missing, which a render trims with the latency.
    const auto latency = linearPhaseEq({})->latencyFrames(48000.0);
    auto cold = linearPhaseEq(testSettings().at(2));
    auto warm = linearPhaseEq(testSettings().at(2));
    const auto coldResponse = impulseResponse(*cold, latency * 2);
    const auto warmResponse = impulseResponse(*warm, latency * 2, latency);
    for (size_t i = latency / 2; i < coldResponse.size(); i++) {
        QVERIFY2(std::abs(coldResponse.at(i) - warmResponse.at(i)) < 1.0e-12, qPrintable(QString::number(i)));
    }
}

void Eq32BandGraphicalTest::test_linearPhase_sine_shouldMatchTheCurve()
{
    // The same magnitude as the minimum-phase bands, which is what the curve shows. Exact on a smooth
    // curve; on random sliders the bottom bands, where neighbours jump by 20 dB, ring for longer than
    // the kernel holds and come out a few tenths off.
    const auto check = [](const Gains & sliders, double lowTolerance) {
        auto eq = linearPhaseEq(sliders);
        for (const double hz : { 20.0, 31.5, 40.0, 63.0, 400.0, 1000.0, 3150.0, 12500.0 }) {
            const double measured = measuredGainDb(*eq, hz);
            const double tolerance = hz < 50.0 ? lowTolerance : 0.05;
            QVERIFY2(std::abs(measured - eq->magnitudeDbAt(hz)) < tolerance, qPrintable(QString { "%1 Hz: %2 dB measured, %3 dB drawn" }.arg(hz).arg(measured).arg(eq->magnitudeDbAt(hz))));
        }
    };
    check(testSettings().at(1), 0.05);
    check(testSettings().at(2), 0.5);
}

void Eq32BandGraphicalTest::test_linearPhase_midMode_shouldDelayTheSideUnchanged()
{
    auto eq = linearPhaseEq(testSettings().at(2), 1);
    const auto latency = eq->latencyFrames(48000.0);
    const size_t frames = latency + 4096;
    std::vector<double> stereo(frames * 2);
    for (size_t i = 0; i < frames; i++) {
        const double sample = std::sin(static_cast<double>(i) * 0.05);
        stereo[i * 2] = sample;
        stereo[i * 2 + 1] = -sample;
    }
    const auto output = render(*eq, stereo);
    for (size_t i = latency; i < frames; i++) {
        QVERIFY(std::abs(output[i * 2] - stereo[(i - latency) * 2]) < 1.0e-12);
        QVERIFY(std::abs(output[i * 2 + 1] - stereo[(i - latency) * 2 + 1]) < 1.0e-12);
    }
}

void Eq32BandGraphicalTest::test_linearPhase_sliderMoved_shouldReachTheNewCurve()
{
    // Moved while audio runs, the kernel is rebuilt a slice at a time and faded in; once it has,
    // the filter is the new curve
    auto eq = linearPhaseEq({});
    render(*eq, std::vector<double>(48000 * 2, 0.0));
    setSliders(*eq, testSettings().at(1));
    render(*eq, std::vector<double>(48000 * 2, 0.0));
    for (const double hz : { 400.0, 3150.0 }) {
        const double measured = measuredGainDb(*eq, hz);
        QVERIFY2(std::abs(measured - eq->magnitudeDbAt(hz)) < 0.1, qPrintable(QString { "%1 Hz: %2 dB" }.arg(hz).arg(measured)));
    }
}

void Eq32BandGraphicalTest::test_linearPhase_sliderMoved_shouldRenderTheSameEveryTime()
{
    // The rebuild is paced by frames, not by the clock, so a render does not depend on the machine
    std::vector<double> stereo(48000 * 2);
    std::mt19937 generator { 4 };
    std::uniform_real_distribution<double> noise { -0.5, 0.5 };
    std::ranges::generate(stereo, [&] { return noise(generator); });
    const auto run = [&stereo]() {
        auto eq = linearPhaseEq({});
        std::vector<double> output;
        for (size_t block = 0; block < 4; block++) {
            if (block == 1) {
                setSliders(*eq, testSettings().at(2));
            }
            const auto rendered = render(*eq, stereo);
            output.insert(output.end(), rendered.begin(), rendered.end());
        }
        return output;
    };
    QCOMPARE(run(), run());
}

void Eq32BandGraphicalTest::test_linearPhase_tail_shouldKeepTheEffectUnsettled()
{
    auto eq = linearPhaseEq(testSettings().at(2));
    QVERIFY(eq->isSettled());
    render(*eq, std::vector<double>(4096 * 2, 0.5));
    QVERIFY(!eq->isSettled());
    // A second of silence is longer than the kernel and a partition either side of it
    render(*eq, std::vector<double>(48000 * 2, 0.0));
    QVERIFY(eq->isSettled());
}

void Eq32BandGraphicalTest::test_effectRack_latency_shouldSumTheEnabledEffects()
{
    EffectRack rack;
    rack.setEffect(0, linearPhaseEq({}));
    rack.setEffect(1, linearPhaseEq({}));
    const auto one = Eq32BandGraphical::linearPhaseLatencyFrames(48000.0);
    QCOMPARE(rack.latencyFrames(48000.0), one * 2);
    rack.effect(1)->setEnabled(false);
    QCOMPARE(rack.latencyFrames(48000.0), one);
}

void Eq32BandGraphicalTest::test_autoGain_minimumPhase_shouldLevelPinkNoise()
{
    Eq32BandGraphical eq;
    eq.setSampleRate(48000.0);
    setSliders(eq, lowBoost());
    const double uncompensated = pinkNoiseGainDb(eq);
    QVERIFY2(uncompensated > 4.0, qPrintable(QString::number(uncompensated)));

    setParameter(eq, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    QVERIFY(eq.autoGainDb() < -4.0);
    const double compensated = pinkNoiseGainDb(eq);
    QVERIFY2(std::abs(compensated) < 0.5, qPrintable(QString::number(compensated)));
}

void Eq32BandGraphicalTest::test_autoGain_linearPhase_shouldLevelPinkNoise()
{
    // Designed into the kernel rather than applied after it, so it has to arrive all the same
    const auto eq = linearPhaseEq(lowBoost());
    setParameter(*eq, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    const double compensated = pinkNoiseGainDb(*eq);
    QVERIFY2(std::abs(compensated) < 0.5, qPrintable(QString::number(compensated)));
}

void Eq32BandGraphicalTest::test_autoGain_on_shouldMatchWhatIsHeard()
{
    for (const bool linearPhase : { false, true }) {
        Eq32BandGraphical eq;
        eq.setSampleRate(48000.0);
        setSliders(eq, lowBoost());
        setParameter(eq, Constants::NahdXml::xmlKeyLinearPhase(), linearPhase ? 1.0f : 0.0f);
        setParameter(eq, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
        for (const double hz : { 100.0, 5000.0 }) {
            const double measured = measuredGainDb(eq, hz);
            const double predicted = eq.magnitudeDbAt(hz);
            QVERIFY2(std::abs(measured - predicted) < 0.5,
                     qPrintable(QString("heard %1 dB, drawn %2 dB at %3 Hz, linear phase %4").arg(measured).arg(predicted).arg(hz).arg(linearPhase)));
        }
    }
}

void Eq32BandGraphicalTest::test_autoGain_midMode_shouldLeaveTheSideAlone()
{
    Eq32BandGraphical eq;
    eq.setSampleRate(48000.0);
    setSliders(eq, lowBoost());
    setParameter(eq, Constants::NahdXml::xmlKeyStereoMode(), 1.0f);
    setParameter(eq, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    QVERIFY(eq.autoGainDb() < -4.0);

    // Pure side content is passed through exactly, not turned down with the mid
    for (int i = 0; i < 4800; i++) {
        double left = 0.2;
        double right = -0.2;
        eq.process(left, right);
        QVERIFY(left == 0.2);
        QVERIFY(right == -0.2);
    }
}

void Eq32BandGraphicalTest::test_autoGain_off_shouldLeaveOutputUnchanged()
{
    // Switched on and back off, the output is bit for bit that of an equalizer that never had it
    Eq32BandGraphical reference;
    Eq32BandGraphical toggled;
    for (auto * eq : { &reference, &toggled }) {
        eq->setSampleRate(48000.0);
        setSliders(*eq, lowBoost());
    }
    const auto run = [&](int frames, bool compare) {
        for (int i = 0; i < frames; i++) {
            const double input = std::sin(static_cast<double>(i) * 0.05);
            double referenceLeft = input;
            double referenceRight = -input * 0.5;
            double toggledLeft = referenceLeft;
            double toggledRight = referenceRight;
            reference.process(referenceLeft, referenceRight);
            toggled.process(toggledLeft, toggledRight);
            if (compare) {
                QVERIFY(toggledLeft == referenceLeft);
                QVERIFY(toggledRight == referenceRight);
            }
        }
    };
    setParameter(toggled, Constants::NahdXml::xmlKeyAutoGain(), 1.0f);
    run(4800, false);
    setParameter(toggled, Constants::NahdXml::xmlKeyAutoGain(), 0.0f);
    QCOMPARE(toggled.autoGainDb(), 0.0);
    // Past the ramp back
    run(960, false);
    run(4800, true);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Eq32BandGraphicalTest)
