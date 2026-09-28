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

#ifndef DRUM_AMP_ENVELOPE_TEST_HPP
#define DRUM_AMP_ENVELOPE_TEST_HPP

#include <QObject>

namespace noteahead {

class DrumAmpEnvelopeTest : public QObject
{
    Q_OBJECT

private slots:
    void test_envelope_beforeTrigger_shouldBeIdleAndSilent();
    void test_envelope_zeroAttack_shouldOpenImmediately();
    void test_envelope_hold_shouldStayOpenForItsTime();
    void test_envelope_decay_shouldReachSilenceAndGoIdle();
    void test_envelope_attack_shouldRiseOverItsTime();
    void test_envelope_curve_shouldMoveTravelToTheStartOfTheDecay();
    void test_envelope_retrigger_duringDecay_shouldChokeThenAttack();
    void test_envelope_retrigger_shouldGiveTheAttackItsFullTime();
    void test_envelope_retrigger_shouldNotStepToSilence();
    void test_envelope_retrigger_fromSilence_shouldAttackAtOnce();
    void test_envelope_reset_shouldGoIdle();
};

} // namespace noteahead

#endif // DRUM_AMP_ENVELOPE_TEST_HPP
