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

#ifndef MONITOR_TEST_HPP
#define MONITOR_TEST_HPP

#include <QObject>

namespace noteahead {

class MonitorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_stereo_shouldPassThroughUntouched();
    void test_mono_shouldSumBothChannelsAtHalf();
    void test_mono_correlatedPair_shouldKeepLevel();
    void test_mono_antiCorrelatedPair_shouldCancel();
    void test_left_shouldPlaceLeftOnBothSides();
    void test_right_shouldPlaceRightOnBothSides();
    void test_side_correlatedPair_shouldCancel();
    void test_side_antiCorrelatedPair_shouldSurvive();
    void test_offline_shouldPassThroughInEveryMode();
    void test_engine_masterInsertRack_shouldFoldOnlyWhenNotOffline();
    void test_engine_deviceInsertRack_shouldFoldOnlyWhenNotOffline();
    void test_sync_shouldUpdateMode();
};

} // namespace noteahead

#endif // MONITOR_TEST_HPP
