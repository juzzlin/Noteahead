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

#ifndef DBTP_METER_TEST_HPP
#define DBTP_METER_TEST_HPP

#include <QObject>

namespace noteahead {

class DbTpMeterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_dbtpMeter_silence_shouldReturnFloor();
    void test_dbtpMeter_fullScale_shouldReturnNearZeroDbtp();
    void test_dbtpMeter_passThrough_shouldNotModifySignal();
    void test_dbtpMeter_reset_shouldClearReadings();
    void test_dbtpMeter_sampleRateChange_shouldReinitialize();
    void test_dbtpMeter_peakHold_shouldRetainMaximum();
};

} // namespace noteahead

#endif // DBTP_METER_TEST_HPP
