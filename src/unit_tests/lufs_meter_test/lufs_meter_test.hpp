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

#ifndef LUFS_METER_TEST_HPP
#define LUFS_METER_TEST_HPP

#include <QObject>

namespace noteahead {

class LufsMeterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_lufsMeter_silence_shouldReturnFloor();
    void test_lufsMeter_sine_shouldMeasureLoudness();
    void test_lufsMeter_passThrough_shouldNotModifySignal();
    void test_lufsMeter_reset_shouldClearReadings();
    void test_lufsMeter_sampleRateChange_shouldReinitialize();
};

} // namespace noteahead

#endif // LUFS_METER_TEST_HPP
