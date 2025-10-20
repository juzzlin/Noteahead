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

#ifndef REAL_TIME_WORKER_POOL_TEST_HPP
#define REAL_TIME_WORKER_POOL_TEST_HPP

#include <QObject>

namespace noteahead {

class RealTimeWorkerPoolTest : public QObject
{
    Q_OBJECT

private slots:
    void test_runExecutesEveryTaskOnce_shouldCompleteAllTasks();
    void test_singleTaskUsesCallerThread_shouldExecuteOnCurrentThread();
    void test_defaultWorkerCount_envOverride_shouldBeRespected();
    void test_run_varyingTaskCounts_shouldRunEveryTaskExactlyOnce();
    void test_run_repeated_shouldNotLoseOrDuplicateTasks();
    void test_run_afterIdlePeriod_shouldWakeSleepingWorkers();
    void test_run_realisticCallbackDuration_shouldNotStall();
    void test_hasRealTimeScheduling_withoutPrivileges_shouldBeFalseNotFatal();
};

} // namespace noteahead

#endif // REAL_TIME_WORKER_POOL_TEST_HPP
