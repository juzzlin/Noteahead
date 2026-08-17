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

#include "adsr_envelope_test.hpp"

#include "../../domain/dsp/adsr_envelope.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };
constexpr double DecaySeconds { 0.05 };
constexpr double ReleaseSeconds { 0.05 };

//! Long enough that the segment is nowhere near over at the points the curve tests measure.
constexpr double LongAttackSeconds { 0.5 };

//! One nextSample() is enough to finish an attack this short, which puts the decay under test on a
//! known sample rather than one lost somewhere inside the attack ramp.
constexpr double InstantAttackSeconds { 0.000001 };

AdsrEnvelope makeEnvelope(double sustainLevel)
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(SampleRate);
    envelope.setAttackTime(0.001);
    envelope.setDecayTime(DecaySeconds);
    envelope.setSustainLevel(sustainLevel);
    envelope.setReleaseTime(ReleaseSeconds);
    return envelope;
}

void advance(AdsrEnvelope & envelope, double seconds)
{
    const int samples = static_cast<int>(seconds * SampleRate);
    for (int i = 0; i < samples; i++) {
        envelope.nextSample();
    }
}

void advanceSamples(AdsrEnvelope & envelope, int samples)
{
    for (int i = 0; i < samples; i++) {
        envelope.nextSample();
    }
}

//! An envelope parked at the very start of its decay, so the samples counted from here are decay
//! samples and nothing else.
AdsrEnvelope makeDecayingEnvelope(double curve, double sustainLevel = 0.0)
{
    auto envelope = makeEnvelope(sustainLevel);
    envelope.setAttackTime(InstantAttackSeconds);
    envelope.setCurve(curve);
    envelope.trigger();
    envelope.nextSample();
    return envelope;
}

double decayLevelAt(double curve, double fraction)
{
    auto envelope = makeDecayingEnvelope(curve);
    advanceSamples(envelope, static_cast<int>(DecaySeconds * SampleRate * fraction));
    return envelope.value();
}

} // namespace

void AdsrEnvelopeTest::test_isSilent_beforeTrigger_shouldBeTrue()
{
    const auto envelope = makeEnvelope(0.5);
    QVERIFY(envelope.isSilent());
}

void AdsrEnvelopeTest::test_isSilent_duringAttack_shouldBeFalse()
{
    // Attack starts from zero on its way up, so a low level there must not read as finished.
    auto envelope = makeEnvelope(0.5);
    envelope.setAttackTime(1.0);
    envelope.trigger();
    envelope.nextSample();

    QVERIFY(!envelope.isSilent());
}

void AdsrEnvelopeTest::test_isSilent_zeroSustain_shouldEndWithoutRelease()
{
    // The percussive case: the envelope decays to silence and parks in Sustain, which is not Idle.
    // Without this, a voice gated on Idle renders digital silence forever, at full cost.
    auto envelope = makeEnvelope(0.0);
    envelope.trigger();
    advance(envelope, 0.5);

    QCOMPARE(envelope.value(), 0.0);
    QVERIFY(envelope.state() != AdsrEnvelope::State::Idle);
    QVERIFY(envelope.isSilent());
}

void AdsrEnvelopeTest::test_isSilent_nonZeroSustain_shouldHoldUntilReleased()
{
    auto envelope = makeEnvelope(0.5);
    envelope.trigger();
    advance(envelope, 0.5);

    QVERIFY(envelope.value() > 0.4);
    QVERIFY(!envelope.isSilent());
}

void AdsrEnvelopeTest::test_isSilent_afterRelease_shouldBeTrue()
{
    auto envelope = makeEnvelope(0.5);
    envelope.trigger();
    advance(envelope, 0.2);
    envelope.release();
    advance(envelope, 0.2);

    QVERIFY(envelope.isSilent());
}

void AdsrEnvelopeTest::test_curve_zeroDecay_shouldStayLinear()
{
    // Zero curve is what the envelope did before it had one, so every project that predates the
    // knob has to come out of the decay on exactly the same straight line.
    constexpr double tolerance { 1.0e-9 };

    QVERIFY(std::abs(decayLevelAt(0.0, 0.25) - 0.75) < tolerance);
    QVERIFY(std::abs(decayLevelAt(0.0, 0.5) - 0.5) < tolerance);
    QVERIFY(std::abs(decayLevelAt(0.0, 0.75) - 0.25) < tolerance);
}

void AdsrEnvelopeTest::test_curve_zeroRelease_shouldStayLinear()
{
    // The release falls from wherever the note was, and takes that fraction of the release time to
    // do it. Halfway through is halfway down.
    auto envelope = makeEnvelope(0.5);
    envelope.setCurve(0.0);
    envelope.trigger();
    advance(envelope, 0.2);
    QCOMPARE(envelope.value(), 0.5);

    envelope.release();
    advanceSamples(envelope, static_cast<int>(ReleaseSeconds * 0.5 * SampleRate * 0.5));

    QVERIFY(std::abs(envelope.value() - 0.25) < 1.0e-9);
}

void AdsrEnvelopeTest::test_curve_halfDecay_shouldMatchPluckShape()
{
    // The middle of the knob has to already be a usable pluck: some 18 dB down at the halfway point
    // of the decay, where the straight line is still only 6 dB down.
    const double level = decayLevelAt(0.5, 0.5);

    QVERIFY(level > 0.05);
    QVERIFY(level < 0.2);
}

void AdsrEnvelopeTest::test_curve_fullDecay_shouldFallFasterThanLinear()
{
    const double level = decayLevelAt(1.0, 0.5);

    QVERIFY(level > 0.0);
    QVERIFY(level < 0.05);
}

void AdsrEnvelopeTest::test_curve_fullDecay_shouldReachSustainAtSameTime()
{
    // Bending the segment must not stretch or cut it: the decay knob keeps meaning the seconds it
    // says, whatever the curve is set to.
    auto envelope = makeDecayingEnvelope(1.0);
    const int decaySamples = static_cast<int>(DecaySeconds * SampleRate);

    advanceSamples(envelope, decaySamples - 2);
    QVERIFY(envelope.value() > 0.0);
    QCOMPARE(envelope.state(), AdsrEnvelope::State::Decay);

    advanceSamples(envelope, 4);
    QCOMPARE(envelope.value(), 0.0);
    QCOMPARE(envelope.state(), AdsrEnvelope::State::Sustain);
}

void AdsrEnvelopeTest::test_curve_fullAttack_shouldRiseFasterThanLinear()
{
    auto envelope = makeEnvelope(1.0);
    envelope.setAttackTime(LongAttackSeconds);
    envelope.setCurve(1.0);
    envelope.trigger();
    advance(envelope, LongAttackSeconds * 0.5);

    QVERIFY(envelope.value() > 0.9);
    QCOMPARE(envelope.state(), AdsrEnvelope::State::Attack);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AdsrEnvelopeTest)
