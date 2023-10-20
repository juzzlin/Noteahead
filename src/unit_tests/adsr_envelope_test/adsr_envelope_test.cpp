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

namespace noteahead {

namespace {

constexpr double SampleRate { 48000.0 };

AdsrEnvelope makeEnvelope(double sustainLevel)
{
    AdsrEnvelope envelope;
    envelope.setSampleRate(SampleRate);
    envelope.setAttackTime(0.001);
    envelope.setDecayTime(0.05);
    envelope.setSustainLevel(sustainLevel);
    envelope.setReleaseTime(0.05);
    return envelope;
}

void advance(AdsrEnvelope & envelope, double seconds)
{
    const int samples = static_cast<int>(seconds * SampleRate);
    for (int i = 0; i < samples; i++) {
        envelope.nextSample();
    }
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AdsrEnvelopeTest)
