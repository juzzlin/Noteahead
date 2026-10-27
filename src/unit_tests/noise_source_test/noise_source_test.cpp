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

#include "noise_source_test.hpp"

#include "../../domain/dsp/fft.hpp"
#include "../../domain/dsp/noise_source.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;
constexpr int FrameSize = 4096;
constexpr int FrameCount = 256;

//! Welch power spectrum of the source, Hann windowed, bins 0..FrameSize/2.
std::vector<double> powerSpectrum(NoiseSource & source)
{
    auto spectrum = std::vector<double>(FrameSize / 2 + 1, 0.0);
    auto frame = std::vector<double>(FrameSize);
    auto re = std::vector<double>(FrameSize / 2 + 1);
    auto im = std::vector<double>(FrameSize / 2 + 1);
    for (int f = 0; f < FrameCount; f++) {
        for (int n = 0; n < FrameSize; n++) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * n / FrameSize);
            frame.at(n) = source.nextSample() * window;
        }
        Fft::forwardReal(frame.data(), re.data(), im.data(), FrameSize);
        for (size_t k = 0; k < spectrum.size(); k++) {
            spectrum.at(k) += re.at(k) * re.at(k) + im.at(k) * im.at(k);
        }
    }
    return spectrum;
}

double meanBandPower(const std::vector<double> & spectrum, double sampleRate, double low, double high)
{
    const auto first = static_cast<size_t>(low / sampleRate * FrameSize);
    const auto last = static_cast<size_t>(high / sampleRate * FrameSize);
    double sum = 0.0;
    for (size_t k = first; k < last; k++) {
        sum += spectrum.at(k);
    }
    return sum / static_cast<double>(last - first);
}

//! Change of the power density from 250-500 Hz to 2-4 kHz, three octaves up, in dB.
double slopeOverThreeOctaves(NoiseSource & source, double sampleRate)
{
    const auto spectrum = powerSpectrum(source);
    return 10.0 * std::log10(meanBandPower(spectrum, sampleRate, 2000.0, 4000.0) / meanBandPower(spectrum, sampleRate, 250.0, 500.0));
}

double rms(NoiseSource & source, int length)
{
    double sum = 0.0;
    for (int n = 0; n < length; n++) {
        const double sample = source.nextSample();
        sum += sample * sample;
    }
    return std::sqrt(sum / length);
}

} // namespace

void NoiseSourceTest::test_noiseSource_spectralSlope_shouldMatchColour_data()
{
    QTest::addColumn<double>("colour");
    QTest::addColumn<double>("expectedDb");

    QTest::newRow("brown") << 0.0 << -18.0;
    QTest::newRow("pink") << 1.0 / 3.0 << -9.0;
    QTest::newRow("white") << 2.0 / 3.0 << 0.0;
    QTest::newRow("blue") << 1.0 << 9.0;
}

void NoiseSourceTest::test_noiseSource_spectralSlope_shouldMatchColour()
{
    QFETCH(double, colour);
    QFETCH(double, expectedDb);

    NoiseSource source { 1 };
    source.setSampleRate(SampleRate);
    source.setColour(colour);

    const double slope = slopeOverThreeOctaves(source, SampleRate);
    QVERIFY2(std::abs(slope - expectedDb) < 1.0, qPrintable(QString::number(slope)));
}

void NoiseSourceTest::test_noiseSource_pinkSlope_highSampleRate_shouldMatchColour()
{
    constexpr double highRate = 96000.0;
    NoiseSource source { 1 };
    source.setSampleRate(highRate);
    source.setColour(1.0 / 3.0);

    const double slope = slopeOverThreeOctaves(source, highRate);
    QVERIFY2(std::abs(slope + 9.0) < 1.0, qPrintable(QString::number(slope)));
}

void NoiseSourceTest::test_noiseSource_level_acrossColours_shouldStayConstant()
{
    constexpr int length = 480000;
    NoiseSource reference { 2 };
    reference.setSampleRate(SampleRate);
    reference.setColour(2.0 / 3.0);
    const double whiteRms = rms(reference, length);

    for (int step = 0; step <= 12; step++) {
        NoiseSource source { 2 };
        source.setSampleRate(SampleRate);
        source.setColour(step / 12.0);
        const double levelDb = 20.0 * std::log10(rms(source, length) / whiteRms);
        QVERIFY2(std::abs(levelDb) < 0.5, qPrintable(QString { "colour %1: %2 dB" }.arg(step / 12.0).arg(levelDb)));
    }
}

void NoiseSourceTest::test_noiseSource_reset_shouldRepeatSequence()
{
    NoiseSource source { 3 };
    source.setSampleRate(SampleRate);
    source.setColour(0.2);

    auto first = std::vector<double>(1000);
    for (auto && sample : first) {
        sample = source.nextSample();
    }
    source.reset();
    for (auto && sample : first) {
        QCOMPARE(source.nextSample(), sample);
    }
}

void NoiseSourceTest::test_noiseSource_seed_different_shouldDecorrelate()
{
    NoiseSource left { 4 };
    NoiseSource right { 5 };
    left.setSampleRate(SampleRate);
    right.setSampleRate(SampleRate);

    double sumLeftRight = 0.0;
    double sumLeft = 0.0;
    double sumRight = 0.0;
    for (int n = 0; n < 480000; n++) {
        const double l = left.nextSample();
        const double r = right.nextSample();
        sumLeftRight += l * r;
        sumLeft += l * l;
        sumRight += r * r;
    }
    QVERIFY(std::abs(sumLeftRight / std::sqrt(sumLeft * sumRight)) < 0.01);
}

void NoiseSourceTest::test_noiseSource_holdRate_shouldHoldValues()
{
    // 750 / 48000 is 2^-6, so the phase adds up exactly and every run is 64 samples long.
    constexpr double holdRate = 750.0;
    constexpr int period = 64;
    NoiseSource source { 6 };
    source.setSampleRate(SampleRate);
    source.setHoldRate(holdRate);

    // The output is zero until the first hold lands.
    double held = 0.0;
    while (held == 0.0) {
        held = source.nextSample();
    }
    for (int run = 0; run < 10; run++) {
        for (int n = 1; n < period; n++) {
            QCOMPARE(source.nextSample(), held);
        }
        const double next = source.nextSample();
        QVERIFY(next != held);
        held = next;
    }
    QCOMPARE(source.holdRate(), std::optional { holdRate });
}

void NoiseSourceTest::test_noiseSource_holdRate_none_shouldPassEverySample()
{
    NoiseSource source { 7 };
    source.setSampleRate(SampleRate);
    QVERIFY(!source.holdRate());

    double previous = source.nextSample();
    for (int n = 0; n < 1000; n++) {
        const double sample = source.nextSample();
        QVERIFY(sample != previous);
        previous = sample;
    }
}

void NoiseSourceTest::test_crackleSource_density_shouldMatchImpulseCount()
{
    CrackleSource source { 8 };
    source.setSampleRate(SampleRate);
    source.setDensity(100.0);

    int count = 0;
    for (int n = 0; n < static_cast<int>(SampleRate * 10.0); n++) {
        if (source.nextSample() != 0.0) {
            count++;
        }
    }
    QVERIFY2(count > 900 && count < 1100, qPrintable(QString::number(count)));
}

void NoiseSourceTest::test_crackleSource_density_zero_shouldBeSilent()
{
    CrackleSource source { 9 };
    source.setSampleRate(SampleRate);

    for (int n = 0; n < 48000; n++) {
        QCOMPARE(source.nextSample(), 0.0);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoiseSourceTest)
