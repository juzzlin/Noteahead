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

#include "real_time_worker_pool_test.hpp"
#include "infra/audio/real_time_worker_pool.hpp"

#include <QTest>

#include <atomic>
#include <vector>

namespace noteahead {

namespace {

struct CountingContext
{
    std::vector<std::atomic<int>> hits;
    std::atomic<int> callerThreadTasks { 0 };
    size_t callerWorkerIndex {};

    explicit CountingContext(size_t taskCount, size_t callerWorkerIndex_)
      : hits(taskCount)
      , callerWorkerIndex { callerWorkerIndex_ }
    {
        for (auto & hit : hits) {
            hit.store(0, std::memory_order_relaxed);
        }
    }
};

void countTask(void * context, size_t taskIndex, size_t workerIndex)
{
    auto & countingContext = *static_cast<CountingContext *>(context);
    countingContext.hits[taskIndex].fetch_add(1, std::memory_order_relaxed);
    if (workerIndex == countingContext.callerWorkerIndex) {
        countingContext.callerThreadTasks.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace

void RealTimeWorkerPoolTest::test_runExecutesEveryTaskOnce_shouldCompleteAllTasks()
{
    constexpr size_t workerCount = 2;
    constexpr size_t taskCount = 256;

    RealTimeWorkerPool pool { workerCount };
    CountingContext context { taskCount, workerCount };

    pool.run(taskCount, &context, countTask);

    for (const auto & hit : context.hits) {
        QCOMPARE(hit.load(std::memory_order_relaxed), 1);
    }
    QVERIFY(context.callerThreadTasks.load(std::memory_order_relaxed) > 0);
}

void RealTimeWorkerPoolTest::test_singleTaskUsesCallerThread_shouldExecuteOnCurrentThread()
{
    RealTimeWorkerPool pool { 2 };
    CountingContext context { 1, 0 };

    pool.run(1, &context, countTask);

    QCOMPARE(context.hits[0].load(std::memory_order_relaxed), 1);
    QCOMPARE(context.callerThreadTasks.load(std::memory_order_relaxed), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RealTimeWorkerPoolTest)
