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

#include "fm_operator_test.hpp"
#include "../../domain/dsp/fm_operator.hpp"

#include <QTest>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>
#include <vector>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

//! A frequency whose period is a whole number of samples at SampleRate: 128 of them. Every test
//! that reasons about periodicity or about a DFT bin needs that, or the window cuts a cycle in half
//! and smears the result across neighbouring bins.
constexpr double Frequency { 375.0 };
constexpr size_t Period { 128 };

//! An operator with its envelope wide open, so what comes out is the oscillator alone.
FmOperator openOperator(FmOperator::Waveform waveform = FmOperator::Waveform::Sine)
{
    FmOperator op;
    op.setSampleRate(SampleRate);
    op.setFrequency(Frequency);
    op.setWaveform(waveform);
    op.envelope().setAttackTime(0.0);
    op.envelope().setDecayTime(0.0);
    op.envelope().setSustainLevel(1.0);
    op.trigger();
    return op;
}

std::vector<double> render(FmOperator & op, size_t count)
{
    std::vector<double> samples;
    samples.reserve(count);
    for (size_t i = 0; i < count; i++) {
        samples.push_back(op.nextSample());
    }
    return samples;
}

//! Renders a carrier modulated by a second operator running at the same frequency, which is the
//! plainest FM pair there is: the sidebands land on harmonics of the note.
std::vector<double> renderPair(double index, size_t count)
{
    auto carrier = openOperator();
    auto modulator = openOperator();
    modulator.setLevel(index);

    std::vector<double> samples;
    samples.reserve(count);
    for (size_t i = 0; i < count; i++) {
        const double mod = modulator.nextSample() * FmOperator::MaxIndexCycles;
        samples.push_back(carrier.nextSample(mod));
    }
    return samples;
}

//! Share of the signal's energy sitting on the fundamental. One means a pure tone; anything less
//! is sidebands. The window must be a whole number of periods for the bin to mean anything.
double fundamentalPurity(const std::vector<double> & samples)
{
    const auto n = static_cast<double>(samples.size());
    const double bin = n / static_cast<double>(Period);
    double re = 0.0;
    double im = 0.0;
    double total = 0.0;
    for (size_t i = 0; i < samples.size(); i++) {
        const double angle = 2.0 * std::numbers::pi * bin * static_cast<double>(i) / n;
        re += samples[i] * std::cos(angle);
        im += samples[i] * std::sin(angle);
        total += samples[i] * samples[i];
    }
    const double magnitude = 2.0 * std::hypot(re, im) / n;
    const double fundamentalEnergy = 0.5 * magnitude * magnitude * n;
    return total > 0.0 ? fundamentalEnergy / total : 0.0;
}

double maximumAbsolute(const std::vector<double> & samples)
{
    double result = 0.0;
    for (const double sample : samples) {
        result = std::max(result, std::abs(sample));
    }
    return result;
}

//! Renders exactly one cycle, one table entry per sample, so the whole waveform is visited and
//! each entry exactly once.
std::vector<double> renderWholeTable(FmOperator::Waveform waveform)
{
    constexpr size_t tableSize = 4096;

    FmOperator op;
    op.setSampleRate(static_cast<double>(tableSize));
    op.setFrequency(1.0);
    op.setWaveform(waveform);
    op.envelope().setAttackTime(0.0);
    op.envelope().setDecayTime(0.0);
    op.envelope().setSustainLevel(1.0);
    op.trigger();

    return render(op, tableSize);
}

//! Largest step between two adjacent table entries of the given waveform.
double maximumStep(FmOperator::Waveform waveform)
{
    const auto samples = renderWholeTable(waveform);
    double result = 0.0;
    for (size_t i = 1; i < samples.size(); i++) {
        result = std::max(result, std::abs(samples[i] - samples[i - 1]));
    }
    // The wrap from the last entry back to the first is an edge like any other.
    return std::max(result, std::abs(samples.front() - samples.back()));
}

} // namespace

void FmOperatorTest::test_nextSample_sine_shouldMatchASine()
{
    auto op = openOperator();
    const auto samples = render(op, Period);

    for (size_t i = 0; i < samples.size(); i++) {
        const double expected = std::sin(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(Period));
        QVERIFY(std::abs(samples[i] - expected) < 1.0e-3);
    }
}

void FmOperatorTest::test_nextSample_unmodulated_shouldBeAPureTone()
{
    auto op = openOperator();
    const auto samples = render(op, Period * 16);

    QVERIFY(fundamentalPurity(samples) > 0.999);
}

void FmOperatorTest::test_nextSample_shouldNotExceedTheLevel()
{
    for (size_t waveform = 0; waveform < FmOperator::WaveformCount; waveform++) {
        auto op = openOperator(static_cast<FmOperator::Waveform>(waveform));
        op.setLevel(0.4);
        const auto samples = render(op, Period * 4);
        QVERIFY(maximumAbsolute(samples) <= 0.4 + 1.0e-9);
    }
}

void FmOperatorTest::test_phaseMod_shouldNotShiftThePitch()
{
    // The whole reason this is phase modulation and not frequency modulation. The modulator runs at
    // the carrier's own frequency, so carrier and modulator together repeat every Period samples --
    // and they still do at an index that would have frequency modulation wandering off the note.
    const auto samples = renderPair(1.0, Period * 8);

    for (size_t i = Period; i < samples.size() - Period; i++) {
        QVERIFY(std::abs(samples[i] - samples[i + Period]) < 1.0e-9);
    }
}

void FmOperatorTest::test_phaseMod_shouldAddSidebands()
{
    const double quiet = fundamentalPurity(renderPair(0.0, Period * 16));
    const double bright = fundamentalPurity(renderPair(1.0, Period * 16));

    QVERIFY(quiet > 0.999);
    QVERIFY(bright < 0.5);
}

void FmOperatorTest::test_phaseMod_constantOffset_shouldOnlyRotateThePhase()
{
    auto plain = openOperator();
    auto offset = openOperator();

    // A quarter of a cycle of constant modulation turns the sine into a cosine and does nothing
    // else. A modulator with a DC component is common -- Abs Sine has a large one -- so a constant
    // offset must not be able to detune anything.
    const auto plainSamples = render(plain, Period * 2);
    std::vector<double> offsetSamples;
    for (size_t i = 0; i < Period * 2; i++) {
        offsetSamples.push_back(offset.nextSample(0.25));
    }

    for (size_t i = 0; i + Period / 4 < plainSamples.size(); i++) {
        QVERIFY(std::abs(offsetSamples[i] - plainSamples[i + Period / 4]) < 1.0e-9);
    }
}

void FmOperatorTest::test_feedback_shouldBrightenTheOutput()
{
    auto plain = openOperator();
    auto fed = openOperator();
    fed.setFeedback(FmOperator::MaxIndexCycles * 0.5);

    QVERIFY(fundamentalPurity(render(fed, Period * 16)) < fundamentalPurity(render(plain, Period * 16)));
}

void FmOperatorTest::test_feedback_halfDepth_shouldStayPeriodic()
{
    auto op = openOperator();
    op.setFeedback(FmOperator::MaxIndexCycles * 0.5);

    // What averaging the last two outputs is for. Half a second in, half way up the knob, the loop
    // is still resolving to a waveform that repeats with the note. Fed off the last sample alone it
    // has been chaotic since a third of the way up -- so this is the test that fails if the
    // averaging is ever dropped.
    const auto samples = render(op, static_cast<size_t>(SampleRate / 2));

    for (size_t i = samples.size() - Period * 8; i < samples.size() - Period; i++) {
        QVERIFY(std::abs(samples[i] - samples[i + Period]) < 1.0e-9);
    }
}

void FmOperatorTest::test_feedback_maximum_shouldStayFinite()
{
    auto op = openOperator();
    op.setFeedback(FmOperator::MaxIndexCycles);

    // Past roughly two thirds of the knob the loop breaks into noise on purpose -- that is where
    // the DX-series gets its noise from, and it is the only noise source this synth has. It still
    // has to stay a bounded, finite signal rather than run away.
    const auto samples = render(op, static_cast<size_t>(SampleRate / 2));

    QVERIFY(maximumAbsolute(samples) <= 1.0);
    for (const double sample : samples) {
        QVERIFY(std::isfinite(sample));
    }
}

void FmOperatorTest::test_level_shouldScaleTheOutput()
{
    auto full = openOperator();
    auto half = openOperator();
    half.setLevel(0.5);

    const auto fullSamples = render(full, Period);
    const auto halfSamples = render(half, Period);

    for (size_t i = 0; i < fullSamples.size(); i++) {
        QVERIFY(std::abs(halfSamples[i] - fullSamples[i] * 0.5) < 1.0e-9);
    }
}

void FmOperatorTest::test_level_zero_shouldSilenceTheFeedbackPath()
{
    auto op = openOperator();
    op.setLevel(0.0);
    op.setFeedback(FmOperator::MaxIndexCycles);

    for (const double sample : render(op, Period * 4)) {
        QCOMPARE(sample, 0.0);
    }
}

void FmOperatorTest::test_envelope_shouldGateTheOutput()
{
    FmOperator op;
    op.setSampleRate(SampleRate);
    op.setFrequency(Frequency);

    // Never triggered: silent, whatever the oscillator is doing.
    for (const double sample : render(op, Period)) {
        QCOMPARE(sample, 0.0);
    }

    op.envelope().setAttackTime(0.0);
    op.envelope().setDecayTime(0.0);
    op.envelope().setSustainLevel(1.0);
    op.trigger();

    QVERIFY(maximumAbsolute(render(op, Period)) > 0.9);
}

void FmOperatorTest::test_isSilent_afterRelease_shouldBecomeTrue()
{
    auto op = openOperator();
    op.envelope().setReleaseTime(0.01);

    QVERIFY(!op.isSilent());

    op.release();
    render(op, static_cast<size_t>(SampleRate * 0.02));

    QVERIFY(op.isSilent());
}

void FmOperatorTest::test_waveformNames_shouldNameEveryWaveform()
{
    QCOMPARE(FmOperator::waveformNames().size(), FmOperator::WaveformCount);
}

void FmOperatorTest::test_waveform_everyOne_shouldStayWithinRange()
{
    for (size_t waveform = 0; waveform < FmOperator::WaveformCount; waveform++) {
        auto op = openOperator(static_cast<FmOperator::Waveform>(waveform));
        QVERIFY(maximumAbsolute(render(op, Period * 4)) <= 1.0 + 1.0e-9);
    }
}

void FmOperatorTest::test_waveform_everyOne_shouldDifferFromTheSine()
{
    auto sine = openOperator();
    const auto reference = render(sine, Period * 2);

    for (size_t waveform = 1; waveform < FmOperator::WaveformCount; waveform++) {
        auto op = openOperator(static_cast<FmOperator::Waveform>(waveform));
        QVERIFY(render(op, Period * 2) != reference);
    }
}

void FmOperatorTest::test_waveform_everyOne_shouldBeZeroMean()
{
    // A carrier's offset is multiplied by the amp envelope and heard as a thump at every note on,
    // and the gated and rectified waveforms are mostly offset by nature. The tables have it taken
    // out, so nothing downstream has to fade it away after the note has already started.
    //
    // Clocked one table entry at a time, so the average is over the whole cycle. Sampled at a
    // musical pitch instead, a waveform with a step in it -- Quarter Sine -- visits too few of its
    // entries for the average to say anything about the table.
    for (size_t waveform = 0; waveform < FmOperator::WaveformCount; waveform++) {
        const auto samples = renderWholeTable(static_cast<FmOperator::Waveform>(waveform));
        const double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
        QVERIFY(std::abs(mean) < 1.0e-3);
    }
}

void FmOperatorTest::test_waveform_squareAndSaw_shouldBeRoundedRatherThanStepped()
{
    // A literal square or saw steps by the whole 2.0 of its range in one sample, and no amount of
    // oversampling band-limits a step. Both are drawn as curves instead, so their steepest edge is
    // a couple of orders of magnitude gentler. Quarter Sine is deliberately not in here: its drop
    // to zero a quarter of the way through is the waveform.
    QVERIFY(maximumStep(FmOperator::Waveform::Square) < 0.02);
    QVERIFY(maximumStep(FmOperator::Waveform::Saw) < 0.02);
}

void FmOperatorTest::test_setFrequency_shouldUpdateThePhaseStep()
{
    auto op = openOperator();
    op.nextSample();
    QVERIFY(std::abs(op.phase() - Frequency / SampleRate) < 1.0e-12);

    op.setFrequency(Frequency * 2.0);
    op.nextSample();
    QVERIFY(std::abs(op.phase() - 3.0 * Frequency / SampleRate) < 1.0e-12);
}

void FmOperatorTest::test_sync_shouldSetThePhase()
{
    auto op = openOperator();
    render(op, 7);

    op.sync(0.25);
    QCOMPARE(op.phase(), 0.25);

    // Out of range wraps rather than clamps: a phase is a position on a circle.
    op.sync(1.75);
    QVERIFY(std::abs(op.phase() - 0.75) < 1.0e-12);
}

void FmOperatorTest::test_reset_shouldClearTheFeedbackHistory()
{
    auto fresh = openOperator();
    fresh.setFeedback(FmOperator::MaxIndexCycles * 0.5);

    auto reused = openOperator();
    reused.setFeedback(FmOperator::MaxIndexCycles * 0.5);
    render(reused, Period * 3);
    reused.reset();
    reused.envelope().setAttackTime(0.0);
    reused.envelope().setDecayTime(0.0);
    reused.envelope().setSustainLevel(1.0);
    reused.trigger();

    QCOMPARE(render(reused, Period), render(fresh, Period));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmOperatorTest)
