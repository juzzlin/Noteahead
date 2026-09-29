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

#ifndef STEREO_LEVEL_METER_TEST_HPP
#define STEREO_LEVEL_METER_TEST_HPP

#include <QObject>

namespace noteahead {

class StereoLevelMeterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_write_inactive_shouldStaySilent();
    void test_write_fullScale_shouldReadZeroDbfsOnBothChannels();
    void test_write_oneSidedSignal_shouldLeaveTheOtherChannelSilent();
    void test_write_differentLevels_shouldKeepTheChannelsApart();
    void test_write_monoBuffer_shouldReadTheSameOnBothChannels();
    void test_write_afterSilence_shouldFallBackThePeak();
    void test_setActive_off_shouldResetTheReadings();
};

} // namespace noteahead

#endif // STEREO_LEVEL_METER_TEST_HPP
