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

#ifndef REVERB_224_TEST_HPP
#define REVERB_224_TEST_HPP

#include <QObject>

namespace noteahead {

class Reverb224Test : public QObject
{
    Q_OBJECT

private slots:
    void test_convert_zero_shouldStayZero();
    void test_convert_quietSignal_shouldKeepItsResolution();
    void test_process_mixZero_shouldLeaveTheSignalAlone();
    void test_processBlock_beforeThePreDelay_shouldBeSilent();
    void test_processBlock_decay_shouldMatchTheSettingInEveryBand();
    void test_processBlock_decay_bassAndMid_shouldDecayApart();
    void test_processBlock_trebleDecay_shouldShortenTheTop();
    void test_processBlock_bandwidth_shouldStopAboveEightKilohertz();
    void test_processBlock_converters_shouldKeepTheLevel();
    void test_processBlock_depth_plates_shouldChangeNothing();
    void test_processBlock_depth_halls_shouldBalanceTheEarlyReflections();
    void test_processBlock_programs_shouldAllDiffer();
    void test_processBlock_tail_shouldBeDense();
    void test_processBlock_longestSettings_shouldStayStable();
    void test_processBlock_44100And96000_shouldSoundTheSame();
    void test_processBlock_rateChange_shouldStartTheNewStreamClean();
    void test_isSettled_shouldWaitForTheTail();
    void test_reset_shouldRenderIdentically();
    void test_factoryPresets_shouldNameRealParameters();
};

} // namespace noteahead

#endif // REVERB_224_TEST_HPP
