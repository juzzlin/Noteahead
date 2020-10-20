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

#include "base_rate_source_test.hpp"

#include "../../domain/dsp/base_rate_source.hpp"
#include "../../domain/dsp/upsampler.hpp"

#include <QTest>

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace noteahead {

namespace {

constexpr double BaseSampleRate { 48000.0 };

//! A naive square bank like the drum engines': its spectrum depends on the rate it runs at, which
//! is exactly why it has to be generated at the base rate.
class SquareBank
{
public:
    explicit SquareBank(double sampleRate)
      : m_sampleRate { sampleRate }
    {
    }

    float nextSample()
    {
        static constexpr std::array<double, 6> ratios { 1.0, 1.47, 1.91, 2.51, 3.39, 4.21 };
        double sum = 0.0;
        for (size_t i = 0; i < ratios.size(); i++) {
            m_phases.at(i) += 825.0 * ratios.at(i) / m_sampleRate;
            if (m_phases.at(i) >= 1.0) {
                m_phases.at(i) -= 1.0;
            }
            sum += m_phases.at(i) < 0.5 ? 1.0 : -1.0;
        }
        return static_cast<float>(sum / static_cast<double>(ratios.size()));
    }

private:
    double m_sampleRate;
    std::array<double, 6> m_phases {};
};

//! Renders the bank through a base-rate source at the given factor and decimates back, the way a
//! drum engine and the audio engine do together.
std::vector<double> render(uint8_t factor, int baseSamples)
{
    SquareBank bank { BaseSampleRate }; // Always the base rate: that is the whole point
    BaseRateSource source;
    source.setOversampleFactor(factor);
    Decimator decimator;

    std::vector<double> out;
    std::array<float, 4> high {};
    for (int n = 0; n < baseSamples; n++) {
        for (uint8_t k = 0; k < factor; k++) {
            if (source.needsBaseSample()) {
                source.setBaseSample(bank.nextSample());
            }
            high.at(k) = source.nextSample();
        }
        out.push_back(decimator.process(high.data(), factor));
    }
    return out;
}

double bandLevel(const std::vector<double> & samples, double low, double high)
{
    constexpr int n = 4096;
    double total = 0.0;
    for (size_t offset = 0; offset + n <= samples.size(); offset += n) {
        for (int bin = static_cast<int>(low * n / BaseSampleRate); bin < static_cast<int>(high * n / BaseSampleRate); bin++) {
            double re = 0.0, im = 0.0;
            for (int i = 0; i < n; i++) {
                const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * i / (n - 1));
                const double angle = 2.0 * std::numbers::pi * bin * i / n;
                re += samples[offset + i] * window * std::cos(angle);
                im -= samples[offset + i] * window * std::sin(angle);
            }
            total += re * re + im * im;
        }
    }
    return total;
}

} // namespace

void BaseRateSourceTest::test_factorOne_shouldPassThrough()
{
    BaseRateSource source;
    source.setOversampleFactor(1);

    for (const float value : { 0.25f, -0.5f, 1.0f }) {
        QVERIFY(source.needsBaseSample());
        source.setBaseSample(value);
        QCOMPARE(source.nextSample(), value);
    }
}

void BaseRateSourceTest::test_needsBaseSample_shouldAskOncePerBaseRateSample()
{
    for (const uint8_t factor : { 1, 2, 4 }) {
        BaseRateSource source;
        source.setOversampleFactor(factor);

        int asked = 0;
        for (int i = 0; i < 40 * factor; i++) {
            if (source.needsBaseSample()) {
                asked++;
                source.setBaseSample(1.0f);
            }
            source.nextSample();
        }
        QCOMPARE(asked, 40);
    }
}

void BaseRateSourceTest::test_squareBank_shouldSoundTheSameAtEveryFactor()
{
    // The regression this guards: a naive oscillator bank run at the oversampled rate changes
    // spectrum with the factor, so the same patch sounds different at Draft, Normal and High.
    const auto reference = render(1, 24000);
    const double bands[][2] = { { 2000, 6000 }, { 6000, 12000 }, { 12000, 20000 } };

    for (const uint8_t factor : { 2, 4 }) {
        const auto rendered = render(factor, 24000);
        for (const auto & band : bands) {
            const double decibels = 10.0 * std::log10(bandLevel(rendered, band[0], band[1]) / bandLevel(reference, band[0], band[1]));
            QVERIFY2(std::abs(decibels) < 0.5,
                     qPrintable(QString { "%1x, %2-%3 Hz: %4 dB" }.arg(factor).arg(band[0]).arg(band[1]).arg(decibels)));
        }
    }
}

void BaseRateSourceTest::test_reset_shouldClearState()
{
    BaseRateSource source;
    source.setOversampleFactor(4);
    source.setBaseSample(1.0f);
    source.nextSample();

    source.reset();

    QVERIFY(source.needsBaseSample());
    source.setBaseSample(0.0f);
    QCOMPARE(source.nextSample(), 0.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::BaseRateSourceTest)
