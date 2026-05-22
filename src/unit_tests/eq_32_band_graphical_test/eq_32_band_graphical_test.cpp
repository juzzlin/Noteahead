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
#include "../../domain/effects/eq_32_band_graphical.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
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
    const auto frames = static_cast<size_t>(rate);
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::Eq32BandGraphicalTest)
