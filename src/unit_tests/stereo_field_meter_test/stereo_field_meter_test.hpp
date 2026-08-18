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

#ifndef STEREO_FIELD_METER_TEST_HPP
#define STEREO_FIELD_METER_TEST_HPP

#include <QObject>

namespace noteahead {

class StereoFieldMeterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_process_meter_shouldPassAudioThrough();

    void test_correlation_monoInput_shouldReadOne();
    void test_correlation_invertedInput_shouldReadMinusOne();
    void test_correlation_decorrelatedInput_shouldReadNearZero();

    void test_bandCorrelation_wideHighsOverMonoLows_shouldSeparateTheBands();

    void test_levels_monoInput_shouldReportSilentSide();
    void test_levels_sideOnlyInput_shouldReportSilentMid();

    void test_balance_leftOnlyInput_shouldReadLeft();
    void test_balance_centredInput_shouldReadCentre();

    void test_analysis_disabled_shouldNotUpdateTheReading();
};

} // namespace noteahead

#endif // STEREO_FIELD_METER_TEST_HPP
