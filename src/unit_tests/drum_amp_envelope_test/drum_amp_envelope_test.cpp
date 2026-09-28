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

#include "drum_amp_envelope_test.hpp"

#include "../../domain/dsp/drum/drum_amp_envelope.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

constexpr double sampleRate = 48000.0;

//! Runs the envelope for the given time and hands back the level it ended on.
double advance(DrumAmpEnvelope & envelope, double seconds)
{
    const auto samples = static_cast<int>(seconds * sampleRate);
    double level = envelope.value();
    for (int i = 0; i < samples; i++) {
        level = envelope.nextSample();
    }
    return level;
}

DrumAmpEnvelope makeEnvelope(double attack, double hold, double decay)
{
    DrumAmpEnvelope envelope;
    envelope.setSampleRate(sampleRate);
    envelope.setAttackTime(attack);
    envelope.setHoldTime(hold);
    envelope.setDecayTime(decay);
    return envelope;
}

} // namespace

void DrumAmpEnvelopeTest::test_envelope_beforeTrigger_shouldBeIdleAndSilent()
{
    auto envelope = makeEnvelope(0.0, 0.1, 0.1);

    QCOMPARE(envelope.state(), DrumAmpEnvelope::State::Idle);
    QVERIFY(!envelope.isActive());
    QCOMPARE(envelope.nextSample(), 0.0);
}

void DrumAmpEnvelopeTest::test_envelope_zeroAttack_shouldOpenImmediately()
{
    // A drum is struck, so the default attack has to be a step rather than a ramp: anything else
    // would take the edge off every transient in the kit.
    auto envelope = makeEnvelope(0.0, 0.1, 0.1);
    envelope.trigger();

    QCOMPARE(envelope.nextSample(), 1.0);
    QVERIFY(envelope.isActive());
}

void DrumAmpEnvelopeTest::test_envelope_hold_shouldStayOpenForItsTime()
{
    auto envelope = makeEnvelope(0.0, 0.1, 0.5);
    envelope.trigger();

    QCOMPARE(advance(envelope, 0.05), 1.0);
    QCOMPARE(envelope.state(), DrumAmpEnvelope::State::Hold);
    QCOMPARE(advance(envelope, 0.04), 1.0);

    // Past the hold, the decay has taken over and the level is on its way down.
    advance(envelope, 0.02);
    QCOMPARE(envelope.state(), DrumAmpEnvelope::State::Decay);
    QVERIFY(envelope.value() < 1.0);
}

void DrumAmpEnvelopeTest::test_envelope_decay_shouldReachSilenceAndGoIdle()
{
    auto envelope = makeEnvelope(0.0, 0.05, 0.1);
    envelope.trigger();

    advance(envelope, 0.2);

    QCOMPARE(envelope.value(), 0.0);
    QCOMPARE(envelope.state(), DrumAmpEnvelope::State::Idle);
    // This is what lets the voice stop being rendered, so a short envelope is cheaper than a long
    // one rather than merely quieter.
    QVERIFY(!envelope.isActive());
}

void DrumAmpEnvelopeTest::test_envelope_attack_shouldRiseOverItsTime()
{
    auto envelope = makeEnvelope(0.1, 0.1, 0.1);
    envelope.trigger();

    const auto quarterWay = advance(envelope, 0.025);
    QVERIFY2(quarterWay > 0.2 && quarterWay < 0.3, qPrintable(QString::number(quarterWay)));
    QCOMPARE(envelope.state(), DrumAmpEnvelope::State::Attack);

    advance(envelope, 0.08);
    QCOMPARE(envelope.value(), 1.0);
}

void DrumAmpEnvelopeTest::test_envelope_curve_shouldMoveTravelToTheStartOfTheDecay()
{
    const auto halfWayLevel = [](double curve) {
        auto envelope = makeEnvelope(0.0, 0.0, 0.2);
        envelope.setCurve(curve);
        envelope.trigger();
        advance(envelope, 0.1);
        return envelope.value();
    };

    // Straight down is still at half level half way through; bent, most of the fall has happened by
    // then, which is what a struck drum actually does.
    const auto straight = halfWayLevel(0.0);
    QVERIFY2(std::abs(straight - 0.5) < 0.01, qPrintable(QString::number(straight)));

    const auto bent = halfWayLevel(1.0);
    QVERIFY2(bent < straight - 0.2, qPrintable(QString { "straight %1, bent %2" }.arg(straight).arg(bent)));
}

void DrumAmpEnvelopeTest::test_envelope_retrigger_duringDecay_shouldRiseFromWhereItStood()
{
    // A drum played fast is retriggered mid-tail, and starting the attack from zero there would put
    // a hole in the sound on every fast roll.
    auto envelope = makeEnvelope(0.02, 0.0, 0.5);
    envelope.trigger();
    advance(envelope, 0.1);
    const auto beforeRetrigger = envelope.value();
    QVERIFY(beforeRetrigger > 0.0 && beforeRetrigger < 1.0);

    envelope.trigger();
    QCOMPARE(envelope.state(), DrumAmpEnvelope::State::Attack);
    QVERIFY2(envelope.nextSample() > beforeRetrigger, "the retrigger dipped below where the tail stood");
}

void DrumAmpEnvelopeTest::test_envelope_reset_shouldGoIdle()
{
    auto envelope = makeEnvelope(0.0, 1.0, 1.0);
    envelope.trigger();
    advance(envelope, 0.1);
    QVERIFY(envelope.isActive());

    envelope.reset();

    QCOMPARE(envelope.state(), DrumAmpEnvelope::State::Idle);
    QCOMPARE(envelope.value(), 0.0);
    QVERIFY(!envelope.isActive());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DrumAmpEnvelopeTest)
