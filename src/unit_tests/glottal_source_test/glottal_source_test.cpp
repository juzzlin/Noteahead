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

#include "glottal_source_test.hpp"

#include "../../domain/dsp/poly_blep_oscillator.hpp"
#include "../../domain/dsp/speech/glottal_source.hpp"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate = 48000.0;
constexpr double Frequency = 200.0;
//! Frames one period lasts at the pitch these tests use. A whole number, deliberately: a test that
//! counts periods should not also be measuring a rounding error.
constexpr size_t PeriodFrames = 240;

GlottalSource makeSource(double openQuotient = 0.5)
{
    GlottalSource source;
    source.setSampleRate(SampleRate);
    source.setFrequency(Frequency);
    source.setOpenQuotient(openQuotient);
    return source;
}

std::vector<double> render(GlottalSource & source, size_t frames)
{
    std::vector<double> out;
    out.reserve(frames);
    for (size_t i = 0; i < frames; i++) {
        out.push_back(source.nextSample());
    }
    return out;
}

//! The frames on which the pulse opens, found by the flow leaving zero.
std::vector<size_t> pulseStarts(const std::vector<double> & samples)
{
    std::vector<size_t> starts;
    for (size_t i = 1; i < samples.size(); i++) {
        if (samples[i - 1] <= 0.0 && samples[i] > 0.0) {
            starts.push_back(i);
        }
    }
    return starts;
}

//! Magnitude of the @p harmonic'th harmonic of the fundamental, by direct correlation.
//!
//! A whole number of periods is correlated against, so no window is needed and no leakage is left
//! for one to clean up -- which matters here, because what is being measured is the *ratio* of two
//! neighbouring harmonics and a window would smear one into the other.
double harmonicMagnitude(const std::vector<double> & samples, size_t harmonic)
{
    const double omega = 2.0 * M_PI * static_cast<double>(harmonic) / static_cast<double>(PeriodFrames);
    double re = 0.0, im = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        re += samples[i] * std::cos(omega * static_cast<double>(i));
        im += samples[i] * std::sin(omega * static_cast<double>(i));
    }
    return std::hypot(re, im) / static_cast<double>(samples.size());
}

//! The steepest rate the flow falls at, which is what rings the tract and so what sets how loud a
//! voice is. Measured as the largest drop between two frames, the pulse having no other fall in it.
double maximumFlowDeclination(const std::vector<double> & samples)
{
    double steepest = 0.0;
    for (size_t i = 1; i < samples.size(); i++) {
        steepest = std::max(steepest, samples[i - 1] - samples[i]);
    }
    return steepest;
}

} // namespace

void GlottalSourceTest::test_pulse_shouldRestBetweenPeriods()
{
    // The closed phase is what makes this a pulse rather than a wave. Without it the folds never
    // meet, and a fold that never meets has nothing to snap shut and so no excitation.
    auto source = makeSource(0.5);
    const auto samples = render(source, PeriodFrames * 4);

    const auto silent = std::ranges::count_if(samples, [](double s) { return s == 0.0; });
    // Half the period is open, so about half of it should be exactly at rest.
    QVERIFY(silent > static_cast<long>(samples.size()) * 4 / 10);
    QVERIFY(silent < static_cast<long>(samples.size()) * 6 / 10);
    // Glottal flow is a volume of air, so it never goes negative.
    QVERIFY(std::ranges::all_of(samples, [](double s) { return s >= 0.0; }));
}

void GlottalSourceTest::test_pulse_shouldRepeatAtTheFundamental()
{
    auto source = makeSource();
    const auto starts = pulseStarts(render(source, PeriodFrames * 8));

    QVERIFY(starts.size() >= 7);
    for (size_t i = 1; i < starts.size(); i++) {
        QCOMPARE(starts[i] - starts[i - 1], PeriodFrames);
    }
}

void GlottalSourceTest::test_openQuotient_shouldSetHowLongTheFoldsAreOpen_data()
{
    QTest::addColumn<double>("openQuotient");

    QTest::newRow("pressed") << 0.3;
    QTest::newRow("modal") << 0.5;
    QTest::newRow("breathy") << 0.8;
}

void GlottalSourceTest::test_openQuotient_shouldSetHowLongTheFoldsAreOpen()
{
    QFETCH(double, openQuotient);

    auto source = makeSource(openQuotient);
    const auto samples = render(source, PeriodFrames * 4);

    const auto open = std::ranges::count_if(samples, [](double s) { return s > 0.0; });
    const double measured = static_cast<double>(open) / static_cast<double>(samples.size());
    // A frame either side of the boundary in each period is the whole of the tolerance.
    QVERIFY2(std::abs(measured - openQuotient) < 0.02, qPrintable(QString::number(measured)));
}

void GlottalSourceTest::test_openQuotient_shouldChangeTheHarmonicBalance()
{
    // The reason the pulse exists at all. The tilt filter it replaces is a uniform slope and cannot
    // move one harmonic relative to its neighbour, so with a sawtooth the whole voice-quality axis
    // -- pressed against breathy, and most of male against female -- was simply not reachable.
    const auto ratio = [](double openQuotient) {
        auto source = makeSource(openQuotient);
        const auto samples = render(source, PeriodFrames * 16);
        return 20.0 * std::log10(harmonicMagnitude(samples, 1) / harmonicMagnitude(samples, 2));
    };

    const double pressed = ratio(0.3);
    const double breathy = ratio(0.8);

    // Open folds put nearly everything in the first harmonic; a short, hard pulse spreads it.
    QVERIFY2(breathy > pressed + 6.0, qPrintable(QString("pressed %1 dB, breathy %2 dB").arg(pressed).arg(breathy)));
}

void GlottalSourceTest::test_openQuotient_shouldNotChangeTheExcitationLevel()
{
    // Openness is a tone control, not a fader. What is held constant is the steepest rate the flow
    // falls at as the folds slam shut, that being the standard predictor of how loud a voice is --
    // and not the pulse's energy, which is a different quantity and the wrong one: normalising that
    // instead measured the five voice types 11 dB apart, because a short pulse carries its energy
    // higher up and so loses less of it to the tilt downstream.
    const auto declination = [](double openQuotient) {
        auto source = makeSource(openQuotient);
        return maximumFlowDeclination(render(source, PeriodFrames * 8));
    };

    const double modal = declination(0.5);
    for (auto && openQuotient : { 0.3, 0.4, 0.6, 0.8 }) {
        const double difference = std::abs(20.0 * std::log10(declination(openQuotient) / modal));
        QVERIFY2(difference < 0.5, qPrintable(QString("OQ %1 is %2 dB off modal").arg(openQuotient).arg(difference)));
    }
}

void GlottalSourceTest::test_openQuotient_shouldBeClampedToWhatAFoldDoes()
{
    // Below the floor the pulse is a handful of samples and turns into a click; above the ceiling
    // there is no closed phase left to snap into.
    auto closed = makeSource(0.0);
    const auto closedSamples = render(closed, PeriodFrames * 4);
    QVERIFY(std::ranges::any_of(closedSamples, [](double s) { return s > 0.0; }));

    auto open = makeSource(1.5);
    const auto openSamples = render(open, PeriodFrames * 4);
    QVERIFY(std::ranges::any_of(openSamples, [](double s) { return s == 0.0; }));
}

void GlottalSourceTest::test_openness_shouldAverageOneOverAPeriod_data()
{
    QTest::addColumn<double>("openQuotient");

    QTest::newRow("pressed") << 0.3;
    QTest::newRow("modal") << 0.5;
    QTest::newRow("breathy") << 0.8;
}

void GlottalSourceTest::test_openness_shouldAverageOneOverAPeriod()
{
    QFETCH(double, openQuotient);

    // Unit mean is what lets the aspiration be pulsed without also being turned down: the caller's
    // breathiness is a level, and it has to go on meaning that level once the noise it scales is
    // being gated by a pulse that is shut for most of the period.
    auto source = makeSource(openQuotient);
    double sum = 0.0;
    const size_t frames = PeriodFrames * 8;
    for (size_t i = 0; i < frames; i++) {
        source.nextSample();
        sum += source.openness();
    }

    QVERIFY2(std::abs(sum / static_cast<double>(frames) - 1.0) < 0.02, qPrintable(QString::number(sum / static_cast<double>(frames))));
}

void GlottalSourceTest::test_openness_shouldFollowThePulse()
{
    // Breath is air through the gap, so it is absent while the folds are shut. Mixed in flat it is
    // a hiss behind a tone rather than the sound of a breathy voice.
    auto source = makeSource(0.5);
    for (size_t i = 0; i < PeriodFrames * 4; i++) {
        const double flow = source.nextSample();
        QCOMPARE(source.openness() == 0.0, flow == 0.0);
    }
}

void GlottalSourceTest::test_jitter_shouldVaryThePeriod()
{
    auto source = makeSource();
    source.setJitter(0.05);
    const auto starts = pulseStarts(render(source, PeriodFrames * 40));

    QVERIFY(starts.size() > 30);
    std::vector<size_t> periods;
    for (size_t i = 1; i < starts.size(); i++) {
        periods.push_back(starts[i] - starts[i - 1]);
    }

    const auto [shortest, longest] = std::ranges::minmax_element(periods);
    QVERIFY2(*shortest < PeriodFrames, "no period came out short");
    QVERIFY2(*longest > PeriodFrames, "no period came out long");
    // The pitch wanders, it does not slide away: a run this long would drift audibly if the
    // perturbation were accumulating rather than being drawn fresh each cycle.
    QVERIFY(*longest - *shortest < PeriodFrames / 2);
}

void GlottalSourceTest::test_jitter_shouldHoldWithinAPeriod()
{
    // A perturbation that moved inside a period would be a modulation rather than the irregularity
    // real folds have, and the pulse it was shaping would not have one length.
    auto source = makeSource();
    source.setJitter(0.05);
    const auto starts = pulseStarts(render(source, PeriodFrames * 20));

    QVERIFY(starts.size() > 15);
    for (size_t i = 1; i < starts.size(); i++) {
        const auto period = starts[i] - starts[i - 1];
        QVERIFY(period > PeriodFrames / 2 && period < PeriodFrames * 2);
    }
}

void GlottalSourceTest::test_shimmer_shouldVaryThePulseHeight()
{
    auto source = makeSource();
    source.setShimmer(0.2);
    const auto samples = render(source, PeriodFrames * 20);

    // Peak of each period, which shimmer is what moves.
    std::vector<double> peaks;
    for (size_t period = 0; period + PeriodFrames <= samples.size(); period += PeriodFrames) {
        peaks.push_back(*std::max_element(samples.begin() + static_cast<long>(period),
                                          samples.begin() + static_cast<long>(period + PeriodFrames)));
    }

    const auto [quietest, loudest] = std::ranges::minmax_element(peaks);
    QVERIFY(*loudest > *quietest * 1.05);
    QVERIFY(*loudest < *quietest * 2.0);
}

void GlottalSourceTest::test_noPerturbation_shouldBePerfectlyPeriodic()
{
    // The default, and what the legacy engine is held to: a project written against the sawtooth
    // has no jitter in it and must not acquire any.
    auto source = makeSource();
    const auto samples = render(source, PeriodFrames * 4);

    // To within the phase accumulator's own drift: the step is a fraction that does not divide the
    // period exactly in binary, so a period is not a whole number of steps and never was.
    for (size_t i = 0; i + PeriodFrames < samples.size(); i++) {
        QVERIFY(std::abs(samples[i + PeriodFrames] - samples[i]) < 1e-6);
    }
}

void GlottalSourceTest::test_sawModel_shouldMatchThePlainOscillator()
{
    // What every project saved before the pulse existed was written against, so it has to be the
    // same waveform and not merely a similar one.
    auto source = makeSource();
    source.setModel(GlottalSource::Model::Saw);

    PolyBlepOscillator reference;
    reference.setSampleRate(SampleRate);
    reference.setWaveform(PolyBlepOscillator::Waveform::Saw);
    reference.setFrequency(Frequency);

    for (size_t i = 0; i < PeriodFrames * 4; i++) {
        QCOMPARE(source.nextSample(), reference.nextSample());
    }
}

void GlottalSourceTest::test_sawModel_openness_shouldBeFlat()
{
    // Flat, so that a caller modulating the aspiration by it gets exactly the steady breath the
    // sawtooth always had.
    auto source = makeSource();
    source.setModel(GlottalSource::Model::Saw);

    for (size_t i = 0; i < PeriodFrames * 4; i++) {
        source.nextSample();
        QCOMPARE(source.openness(), 1.0);
    }
}

void GlottalSourceTest::test_reset_shouldRepeatTheSameOutput()
{
    // The jitter is drawn from a seeded generator so that rendering a project twice gives the same
    // file. Reset has to put the generator back or the second render differs from the first.
    auto source = makeSource();
    source.setJitter(0.05);
    source.setShimmer(0.2);

    const auto first = render(source, PeriodFrames * 8);
    source.reset();
    const auto second = render(source, PeriodFrames * 8);

    QCOMPARE(second, first);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::GlottalSourceTest)
