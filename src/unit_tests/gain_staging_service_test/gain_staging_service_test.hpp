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

#ifndef GAIN_STAGING_SERVICE_TEST_HPP
#define GAIN_STAGING_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class GainStagingServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_step_rms_shouldConvergeToTheTarget();
    void test_step_trim_shouldShiftTheTarget();
    void test_step_peak_shouldSettleOnTheLoudestHitOfTheSession();
    void test_step_silence_shouldLeaveGainAlone();
    void test_step_quietSource_shouldClampGainAtTheTopOfItsRange();
    void test_start_shouldForgetWhatPlayedBeforeThePress();
    void test_stop_shouldEndTheSessionAndReportWhetherGainMoved();
    void test_trimAndMode_shouldRoundTripThroughXml();
};

} // namespace noteahead

#endif // GAIN_STAGING_SERVICE_TEST_HPP
