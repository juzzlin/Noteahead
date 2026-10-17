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

#ifndef PARAMETER_MODULATION_TEST_HPP
#define PARAMETER_MODULATION_TEST_HPP

#include <QObject>

namespace noteahead {

class ParameterModulationTest : public QObject
{
    Q_OBJECT

private slots:
    void test_modulatedValue_bipolar_shouldSwingAroundTheSetting();
    void test_modulatedValue_unipolar_shouldScaleTowardsTheRest();
    void test_modulatedValue_unipolarCentred_shouldScaleTowardsTheCentre();
    void test_modulatedValue_tremolo_shouldScaleTheFadersGain();
    void test_modulatedValue_tremoloOnAnythingElse_shouldBeUnipolar();
    void test_modulatedValue_shouldStayInRange();

    void test_deviceModulation_fader_shouldMoveTheGainAndNotTheKnob();
    void test_deviceModulation_shouldSurviveStop();
    void test_deviceModulation_clear_shouldRestoreTheSetting();
    void test_deviceModulation_switch_shouldBeRefused();
    void test_effectModulation_shouldReachTheDsp();
};

} // namespace noteahead

#endif // PARAMETER_MODULATION_TEST_HPP
