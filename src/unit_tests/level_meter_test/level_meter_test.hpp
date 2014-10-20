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

#ifndef LEVEL_METER_TEST_HPP
#define LEVEL_METER_TEST_HPP

#include <QObject>

namespace noteahead {

class LevelMeterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_write_inactive_shouldStaySilent();
    void test_write_fullScale_shouldReadZeroDbfs();
    void test_write_minusEighteen_shouldReadTheTarget();
    void test_write_sine_shouldSeparatePeakFromRms();
    void test_peak_shouldFallBackWhenSignalStops();
    void test_reset_shouldClearLevels();
    void test_setActive_false_shouldClearLevels();
};

} // namespace noteahead

#endif // LEVEL_METER_TEST_HPP
