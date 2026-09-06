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

#ifndef FM_OPERATOR_TEST_HPP
#define FM_OPERATOR_TEST_HPP

#include <QObject>

namespace noteahead {

class FmOperatorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_nextSample_sine_shouldMatchASine();
    void test_nextSample_unmodulated_shouldBeAPureTone();
    void test_nextSample_shouldNotExceedTheLevel();

    void test_phaseMod_shouldNotShiftThePitch();
    void test_phaseMod_shouldAddSidebands();
    void test_phaseMod_constantOffset_shouldOnlyRotateThePhase();

    void test_feedback_shouldBrightenTheOutput();
    void test_feedback_halfDepth_shouldStayPeriodic();
    void test_feedback_maximum_shouldStayFinite();

    void test_level_shouldScaleTheOutput();
    void test_level_zero_shouldSilenceTheFeedbackPath();

    void test_envelope_shouldGateTheOutput();
    void test_isSilent_afterRelease_shouldBecomeTrue();

    void test_waveformNames_shouldNameEveryWaveform();
    void test_waveform_everyOne_shouldStayWithinRange();
    void test_waveform_everyOne_shouldDifferFromTheSine();
    void test_waveform_everyOne_shouldBeZeroMean();
    void test_waveform_squareAndSaw_shouldBeRoundedRatherThanStepped();

    void test_setFrequency_shouldUpdateThePhaseStep();
    void test_sync_shouldSetThePhase();
    void test_reset_shouldClearTheFeedbackHistory();
};

} // namespace noteahead

#endif // FM_OPERATOR_TEST_HPP
