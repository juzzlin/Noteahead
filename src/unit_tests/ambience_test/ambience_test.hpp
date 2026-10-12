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

#ifndef AMBIENCE_TEST_HPP
#define AMBIENCE_TEST_HPP

#include <QObject>

namespace noteahead {

class AmbienceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_process_mixZero_shouldLeaveTheSignalAlone();
    void test_processBlock_beforeThePreDelay_shouldBeSilent();
    void test_processBlock_decay_shouldMatchTheSettingAtEveryRate();
    void test_processBlock_tail_shouldBeDenseAfterTwentyMilliseconds();
    void test_processBlock_modulation_shouldBreakUpTheRinging();
    void test_processBlock_stereo_shouldBeWideButMonoSafe();
    void test_processBlock_lowCut_shouldTakeTheLowsOutOfTheSpace();
    void test_processBlock_highCut_shouldTakeTheTopOutOfTheSpace();
    void test_processBlock_longestSettings_shouldStayStable();
    void test_processBlock_44100And48000_shouldSoundTheSame();
    void test_processBlock_rateChange_shouldStartTheNewStreamClean();
    void test_processBlock_highCutNearNyquist_shouldStayStableAt44100();
    void test_isSettled_shouldWaitForTheTail();
    void test_reset_shouldSilenceTheTail();
    void test_factoryPresets_shouldNameRealParameters();
};

} // namespace noteahead

#endif // AMBIENCE_TEST_HPP
