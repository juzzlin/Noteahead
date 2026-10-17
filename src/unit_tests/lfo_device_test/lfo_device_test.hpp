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

#ifndef LFO_DEVICE_TEST_HPP
#define LFO_DEVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class LfoDeviceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_typeId_shouldBeAModulator();
    void test_evaluate_withoutTarget_shouldSendNothing();
    void test_evaluate_shouldStepOnTheMillisecondGrid();
    void test_evaluate_anyBlockSize_shouldGiveTheSameSteps();
    void test_evaluate_unchangedValue_shouldSendNothing();
    void test_rateDivider_shouldSlowTheLfo();
    void test_sync_quarterNote_shouldRunAtTheBeat();
    void test_retrigger_note_shouldRestartAtItsStep();
    void test_retrigger_off_shouldIgnoreNotes();
    void test_velocitySensitivity_shouldScaleTheDepth();
    void test_setTarget_shouldReleaseTheOldTarget();
    void test_smoothing_shouldSoftenASquare();
    void test_phase_shouldOffsetTheStart();
    void test_xmlSerialization_shouldRestoreTargetsAndControls();
    void test_xmlSerialization_untouched_shouldWriteNoTargets();
    void test_automatableParameters_shouldBeGroupedByLfo();
    void test_lfoLed_shouldFollowTheShape();
};

} // namespace noteahead

#endif // LFO_DEVICE_TEST_HPP
