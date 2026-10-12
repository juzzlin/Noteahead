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

#ifndef FM_DRUM_ENGINE_TEST_HPP
#define FM_DRUM_ENGINE_TEST_HPP

#include <QObject>

namespace noteahead {

class FmDrumEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void test_trigger_defaultVoice_shouldPlayTheBaseFrequency();
    void test_trigger_twice_shouldRenderTheSameHit();
    void test_trigger_whileSounding_shouldNotStep();

    void test_pitchEnvelope_positiveAmount_shouldFallBackToTheBase();
    void test_pitchEnvelope_negativeAmount_shouldStartBelowTheBase();

    void test_isActive_afterTheDecay_shouldGoFalse();
    void test_isActive_silentCarriers_shouldGoFalseAtOnce();
    void test_isActive_modulatorStillRinging_shouldNotKeepTheDrumAlive();

    void test_stop_shouldFadeOutWithoutAStep();

    void test_algorithm_modulatedCarrier_shouldAddSidebands();

    void test_noise_zeroAmounts_shouldLeaveTheToneUntouched();
    void test_noise_modulation_shouldRoughenTheTone();
    void test_noise_oversampled_shouldKeepItsLevel();

    void test_velocity_levelAmountZero_shouldIgnoreVelocity();
    void test_velocity_softHit_shouldBeQuieter();
    void test_velocity_modulationAmount_softHit_shouldBeDuller();
};

} // namespace noteahead

#endif // FM_DRUM_ENGINE_TEST_HPP
