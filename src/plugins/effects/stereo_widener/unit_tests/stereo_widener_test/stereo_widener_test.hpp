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

#ifndef STEREO_WIDENER_TEST_HPP
#define STEREO_WIDENER_TEST_HPP

#include <QObject>

namespace noteahead {

class StereoWidenerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_width_default_shouldLeaveSignalAlone();
    void test_width_unity_shouldPreserveSideEnergy();
    void test_width_unity_shouldPreserveMidEnergy();
    void test_width_zero_shouldSumToMono();
    void test_width_doubled_shouldDoubleSideAmplitude();
    void test_width_zero_shouldPreserveMidEnergy();

    void test_monoBass_enabled_shouldCentreLowFrequencies();
    void test_monoBass_disabled_shouldLeaveLowFrequenciesAlone();
    void test_monoBass_enabled_shouldLeaveHighFrequenciesAlone();

    void test_bandSolo_enabled_shouldPassOnlySoloedBand();

    void test_bandCorrelation_monoInput_shouldReadCentred();
    void test_bandCorrelation_widenedInput_shouldReadBelowCentred();
};

} // namespace noteahead

#endif // STEREO_WIDENER_TEST_HPP
