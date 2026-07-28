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

#ifndef AIR_BAND_EQ_TEST_HPP
#define AIR_BAND_EQ_TEST_HPP

#include <QObject>

namespace noteahead {

class AirBandEqTest : public QObject
{
    Q_OBJECT

private slots:
    void test_neutral_defaults_shouldPassThrough();
    void test_bandPass_boosted_shouldAmplifyCenterFrequency();
    void test_bandPass_cut_shouldAttenuateCenterFrequency();
    void test_bandPass_fullBoost_shouldReachDocumentedMaximum();
    void test_bandPass_fullCut_shouldStopAtDocumentedMinimum();
    void test_bandPasses_loweredTogether_shouldPreserveCurveShape();
    void test_airBand_boosted_shouldAmplifyHighFrequencies();
    void test_airBand_boosted_shouldRaiseOverallGain();
    void test_airBand_off_shouldPassThrough();
    void test_airBand_skirt_shouldReachBelowSelectedCorner();
    void test_airBand_risingFrequency_shouldReduceAudibleLift();
    void test_airBand_fortyKilohertz_shouldStayStableAtBaseRate();
    void test_outputGain_boosted_shouldScaleOutput();
};

} // namespace noteahead

#endif // AIR_BAND_EQ_TEST_HPP
