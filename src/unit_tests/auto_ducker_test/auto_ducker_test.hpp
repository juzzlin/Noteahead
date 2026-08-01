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

#ifndef AUTO_DUCKER_TEST_HPP
#define AUTO_DUCKER_TEST_HPP

#include <QObject>

namespace noteahead {

class AutoDuckerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_gain_belowThreshold_shouldStayUnity();
    void test_gain_negativeAmount_shouldDuck();
    void test_gain_positiveAmount_shouldBoost();
    void test_gain_knee_shouldEngagePartially();
    void test_sideChain_loudSource_shouldDuckSilentInput();
    void test_sideChain_silentSource_shouldLeaveInputAlone();
    void test_sideChainSourceDeviceIndex_unset_shouldBeEmpty();
    void test_hold_afterSourceStops_shouldDelayRelease();
    void test_reset_afterDucking_shouldReturnToUnity();
};

} // namespace noteahead

#endif // AUTO_DUCKER_TEST_HPP
