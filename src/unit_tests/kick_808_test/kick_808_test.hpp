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

#ifndef KICK_808_TEST_HPP
#define KICK_808_TEST_HPP

#include <QObject>

namespace noteahead {

class Kick808Test : public QObject
{
    Q_OBJECT

private slots:
    void test_midiNoteOn_shouldActivateAudio();
    void test_midiNoteOff_shouldNotStopRinging();
    void test_allNotesOff_shouldChokeVoice();
    void test_decay_short_shouldProduceShorterTailThanLong();
    void test_decay_full_shouldRingForSeconds();
    void test_decay_half_shouldStillRingAfterAQuarterSecond();
    void test_pitchEnvelope_default_shouldStartWellAboveTheNote();
    void test_pitchEnvelope_default_shouldSettleWithinTwentyFiveMilliseconds();
    void test_peakLevel_anySetting_shouldLeaveHeadroom();
    void test_click_brightest_shouldKeepFallingTowardsNyquist();
    void test_keyTrack_enabled_shouldFollowNotePitch();
    void test_keyTrack_disabled_shouldIgnoreNotePitch();
    void test_tune_raised_shouldRaisePitch();
    void test_tone_full_shouldRaiseHighFrequencyContent();
    void test_tone_zero_shouldStillClick();
    void test_velocity_shouldAffectOutputLevel();
    void test_retrigger_shouldStayContinuous();
    void test_retrigger_shouldProduceConsistentHits();
    void test_lpfCutoff_closed_shouldAttenuateOutput();
    void test_hpfCutoff_closed_shouldAttenuateOutput();
    void test_midiCc_shouldReachEveryParameter();
    void test_drive_full_shouldLeaveHeadroom();
    void test_serialization_shouldRestoreParameters();
};

} // namespace noteahead

#endif // KICK_808_TEST_HPP
