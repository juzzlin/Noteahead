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

#ifndef SESSION_TIMER_SERVICE_TEST_HPP
#define SESSION_TIMER_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class SessionTimerServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void test_tick_active_shouldCountSessionAndTotal();
    void test_tick_shortTicks_shouldCarryPartSeconds();
    void test_tick_afterTimeout_shouldGoIdleAndStop();
    void test_tick_timeoutSetting_shouldBeRespected();
    void test_tick_longGap_shouldNotCount();
    void test_tick_shouldNotMarkModified();

    void test_notifyActivity_afterIdle_shouldResumeWithoutTheGap();
    void test_setPlaying_withoutInput_shouldKeepCounting();

    void test_songChanged_shouldStartANewSessionWithTheSongsTotal();
    void test_resetTotal_shouldZeroAndMarkModified();
};

} // namespace noteahead

#endif // SESSION_TIMER_SERVICE_TEST_HPP
