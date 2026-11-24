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

#include "glottal_source_v2_test.hpp"

#include "../../domain/dsp/fft.hpp"
#include "../../domain/dsp/one_pole_filter.hpp"
#include "../../domain/dsp/speech/glottal_source.hpp"
#include "../../domain/dsp/speech/glottal_source_v2.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;
constexpr double Frequency = 200.0;
constexpr size_t PeriodFrames = 240;

GlottalSourceV2 makeSource(double openQuotient = 0.5, double returnPhase = 0.010)
{
    GlottalSourceV2 source {};
    source.setSampleRate(SampleRate);
    source.setFrequency(Frequency);
    source.setOpenQuotient(openQuotient);
    source.setReturnPhase(returnPhase);
    return source;
}

std::vector<double> render(GlottalSourceV2 & source, size_t frames)
{
    std::vector<double> out;
    out.reserve(frames);
    for (size_t i = 0; i < frames; i++) {
        out.push_back(source.nextSample());
    }
    return out;
}

std::vector<size_t> pulseStarts(const std::vector<double> & samples)
{
    std::vector<size_t> starts;
    for (size_t i = 1; i < samples.size(); i++) {
        if (samples[i - 1] <= 0.01 && samples[i] > 0.01) {
            starts.push_back(i);
        }
    }
    return starts;
}

double harmonicMagnitude(const std::vector<double> & samples, size_t harmonic)
{
    const double omega = 2.0 * M_PI * static_cast<double>(harmonic) / static_cast<double>(PeriodFrames);
    double re = 0.0;
    double im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        re += samples[i] * std::cos(omega * static_cast<double>(i));
        im += samples[i] * std::sin(omega * static_cast<double>(i));
    }
    return std::hypot(re, im) / static_cast<double>(samples.size());
}

double maximumFlowDeclination(const std::vector<double> & samples)
{
    double steepest = 0.0;
    for (size_t i = 1; i < samples.size(); i++) {
        steepest = std::max(steepest, samples[i - 1] - samples[i]);
    }
    return steepest;
}

} // namespace

void GlottalSourceV2Test::test_pulse_shouldBeNonNegativeAndContinuous()
{
    auto source = makeSource(0.5, 0.012);
    const auto samples = render(source, PeriodFrames * 4);

    QVERIFY(std::ranges::all_of(samples, [](double s) { return std::isfinite(s) && s >= 0.0; }));
    const auto restSamples = std::ranges::count_if(samples, [](double s) { return s < 0.01; });
    QVERIFY(restSamples > static_cast<long>(samples.size()) / 4);
}

void GlottalSourceV2Test::test_pulse_shouldRepeatAtFundamental()
{
    auto source = makeSource();
    const auto starts = pulseStarts(render(source, PeriodFrames * 8));

    QVERIFY(starts.size() >= 7);
    for (size_t i = 1; i < starts.size(); i++) {
        QCOMPARE(starts[i] - starts[i - 1], PeriodFrames);
    }
}

void GlottalSourceV2Test::test_openQuotient_shouldChangeHarmonicBalance()
{
    const auto ratio = [](double openQuotient) {
        auto source = makeSource(openQuotient);
        const auto samples = render(source, PeriodFrames * 16);
        return 20.0 * std::log10(harmonicMagnitude(samples, 1) / std::max(1.0e-12, harmonicMagnitude(samples, 2)));
    };

    const double pressed = ratio(0.3);
    const double breathy = ratio(0.8);
    QVERIFY2(breathy > pressed + 6.0, qPrintable(QString { "pressed %1 dB, breathy %2 dB" }.arg(pressed).arg(breathy)));
}

void GlottalSourceV2Test::test_returnPhase_shouldControlSpectralTilt()
{
    const auto upperTiltRatio = [](double returnPhase) {
        auto source = makeSource(0.5, returnPhase);
        const auto raw = render(source, PeriodFrames * 16);
        OnePoleFilter radiation {};
        radiation.calculate(4500.0, SampleRate);
        std::vector<double> radiated(raw.size(), 0.0);
        for (size_t i = 0; i < raw.size(); i++) {
            radiation.process(raw[i]);
            radiated[i] = radiation.highPass();
        }
        const double h1 = harmonicMagnitude(radiated, 1);
        double upperPower = 0.0;
        for (size_t h = 8; h <= 20; h++) {
            const double mag = harmonicMagnitude(radiated, h);
            upperPower += mag * mag;
        }
        return 10.0 * std::log10(std::max(1.0e-18, upperPower) / std::max(1.0e-18, h1 * h1));
    };

    const double bright = upperTiltRatio(0.006);
    const double soft = upperTiltRatio(0.060);
    QVERIFY2(bright > soft + 8.0, qPrintable(QString { "bright=%1 dB vs soft=%2 dB" }.arg(bright).arg(soft)));
}

void GlottalSourceV2Test::test_highPitch_shouldAliasFarLessThanRosenberg()
{
    constexpr double highHz = 660.0;
    constexpr int fftSize = 16384;
    constexpr size_t skip = 2000;

    const auto aliasingRatioDb = [&](auto & source) {
        source.setSampleRate(SampleRate);
        source.setFrequency(highHz);
        source.setOpenQuotient(0.35);
        OnePoleFilter radiation {};
        radiation.calculate(4500.0, SampleRate);

        std::vector<double> re(fftSize, 0.0);
        std::vector<double> im(fftSize, 0.0);
        for (size_t i = 0; i < skip; i++) {
            radiation.process(source.nextSample());
        }
        for (int i = 0; i < fftSize; i++) {
            radiation.process(source.nextSample());
            const double window = 0.5 - 0.5 * std::cos(2.0 * M_PI * i / (fftSize - 1));
            re[static_cast<size_t>(i)] = radiation.highPass() * window;
        }
        Fft::forward(re.data(), im.data(), fftSize);

        double harmPower = 0.0;
        double aliasPower = 0.0;
        const size_t minBin = static_cast<size_t>(200.0 * fftSize / SampleRate);
        const size_t maxBin = static_cast<size_t>(8000.0 * fftSize / SampleRate);
        for (size_t i = minBin; i <= maxBin; i++) {
            const double hz = static_cast<double>(i) * SampleRate / fftSize;
            const double nearest = std::round(hz / highHz) * highHz;
            const double power = re[i] * re[i] + im[i] * im[i];
            if (std::abs(hz - nearest) <= 120.0) {
                harmPower += power;
            } else {
                aliasPower += power;
            }
        }
        return 10.0 * std::log10(std::max(1.0e-18, aliasPower) / std::max(1.0e-18, harmPower));
    };

    GlottalSource rosenberg {};
    rosenberg.setModel(GlottalSource::Model::Rosenberg);
    GlottalSourceV2 lf {};

    const double rosDb = aliasingRatioDb(rosenberg);
    const double lfDb = aliasingRatioDb(lf);
    QVERIFY2(lfDb < rosDb - 10.0, qPrintable(QString { "LF=%1 dB vs Rosenberg=%2 dB" }.arg(lfDb).arg(rosDb)));
}

void GlottalSourceV2Test::test_openness_shouldHaveUnitMeanAcrossOpenQuotients()
{
    for (const double openQuotient : { 0.3, 0.5, 0.8 }) {
        auto source = makeSource(openQuotient);
        double sum = 0.0;
        const size_t frames = PeriodFrames * 8;
        for (size_t i = 0; i < frames; i++) {
            source.nextSample();
            sum += source.openness();
        }
        const double mean = sum / static_cast<double>(frames);
        QVERIFY2(std::abs(mean - 1.0) < 0.02, qPrintable(QString { "OQ %1 mean=%2" }.arg(openQuotient).arg(mean)));
    }
}

void GlottalSourceV2Test::test_subharmonicAndPerturbation_shouldProduceGrowlAndVariation()
{
    const auto halfHarmonic = [](double subharmonic) {
        auto source = makeSource();
        source.setSubharmonic(subharmonic);
        const auto samples = render(source, PeriodFrames * 16);
        const double omega = M_PI / static_cast<double>(PeriodFrames);
        double re = 0.0;
        double im = 0.0;
        for (size_t i = 0; i < samples.size(); i++) {
            re += samples[i] * std::cos(omega * static_cast<double>(i));
            im += samples[i] * std::sin(omega * static_cast<double>(i));
        }
        return std::hypot(re, im) / static_cast<double>(samples.size());
    };
    QVERIFY(halfHarmonic(0.6) > std::max(1.0e-6, halfHarmonic(0.0) * 10.0));

    auto jittered = makeSource();
    jittered.setJitter(0.05);
    const auto starts = pulseStarts(render(jittered, PeriodFrames * 32));
    QVERIFY(starts.size() > 20);
    size_t minP = PeriodFrames * 2;
    size_t maxP = 0;
    for (size_t i = 1; i < starts.size(); i++) {
        const size_t p = starts[i] - starts[i - 1];
        minP = std::min(minP, p);
        maxP = std::max(maxP, p);
    }
    QVERIFY(maxP > minP + 4);

    auto shimmered = makeSource();
    shimmered.setShimmer(0.2);
    const auto samples = render(shimmered, PeriodFrames * 16);
    std::vector<double> peaks;
    for (size_t p = 0; p + PeriodFrames <= samples.size(); p += PeriodFrames) {
        peaks.push_back(*std::max_element(samples.begin() + static_cast<long>(p),
                                          samples.begin() + static_cast<long>(p + PeriodFrames)));
    }
    const auto [q, l] = std::ranges::minmax_element(peaks);
    QVERIFY(*l > *q * 1.05);
}

void GlottalSourceV2Test::test_loudnessNormalization_shouldStaySteadyAcrossShapes()
{
    const auto declination = [](double openQuotient) {
        auto source = makeSource(openQuotient, 0.010);
        return maximumFlowDeclination(render(source, PeriodFrames * 8));
    };

    const double modal = declination(0.5);
    for (const double openQuotient : { 0.3, 0.4, 0.6, 0.8 }) {
        const double diffDb = std::abs(20.0 * std::log10(declination(openQuotient) / modal));
        QVERIFY2(diffDb < 1.0, qPrintable(QString { "OQ %1 diff=%2 dB" }.arg(openQuotient).arg(diffDb)));
    }
}

void GlottalSourceV2Test::test_reset_shouldBeDeterministic()
{
    auto source = makeSource();
    source.setJitter(0.05);
    source.setShimmer(0.2);

    const auto first = render(source, PeriodFrames * 8);
    source.reset();
    const auto second = render(source, PeriodFrames * 8);

    QCOMPARE(second, first);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::GlottalSourceV2Test)
