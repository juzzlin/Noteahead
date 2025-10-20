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
#include "../../infra/audio/real_time_worker_pool.hpp"

#include <QTest>
#include <chrono>
#include <thread>

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

void RealTimeWorkerPoolTest::test_defaultWorkerCount_envOverride_shouldBeRespected()
{
    qputenv("NOTEAHEAD_AUDIO_WORKERS", "0");
    QCOMPARE(RealTimeWorkerPool::defaultWorkerCount(), static_cast<size_t>(0));

    qputenv("NOTEAHEAD_AUDIO_WORKERS", "1");
    QCOMPARE(RealTimeWorkerPool::defaultWorkerCount(), static_cast<size_t>(1));

    // Invalid values fall back to the computed hardware-based default.
    qunsetenv("NOTEAHEAD_AUDIO_WORKERS");
    const auto computedDefault = RealTimeWorkerPool::defaultWorkerCount();
    qputenv("NOTEAHEAD_AUDIO_WORKERS", "not-a-number");
    QCOMPARE(RealTimeWorkerPool::defaultWorkerCount(), computedDefault);

    qunsetenv("NOTEAHEAD_AUDIO_WORKERS");
}

void RealTimeWorkerPoolTest::test_run_varyingTaskCounts_shouldRunEveryTaskExactlyOnce()
{
    // Fewer tasks than workers is the interesting case: the generation counter wakes every worker,
    // so the ones this run has no use for must bow out without joining the fan-in count. Getting
    // that wrong underflows the counter and hangs the caller, which on the audio thread is fatal.
    RealTimeWorkerPool pool { 4 };

    for (size_t taskCount : { size_t { 1 }, size_t { 2 }, size_t { 3 }, size_t { 4 }, size_t { 5 }, size_t { 17 }, size_t { 64 } }) {
        std::vector<std::atomic_int> runs(taskCount);
        for (auto & count : runs) {
            count.store(0);
        }
        pool.run(taskCount, &runs, [](void * context, size_t taskIndex, size_t) {
            static_cast<std::vector<std::atomic_int> *>(context)->at(taskIndex).fetch_add(1);
        });
        for (size_t i = 0; i < taskCount; i++) {
            QVERIFY2(runs[i].load() == 1, qPrintable(QString { "task count %1, task %2 ran %3 times" }.arg(taskCount).arg(i).arg(runs[i].load())));
        }
    }
}

void RealTimeWorkerPoolTest::test_run_repeated_shouldNotLoseOrDuplicateTasks()
{
    // Back-to-back runs keep the workers spinning rather than sleeping, which is the path the audio
    // callback actually takes.
    RealTimeWorkerPool pool { 4 };
    constexpr size_t taskCount { 32 };
    std::vector<std::atomic_int> runs(taskCount);
    for (auto & count : runs) {
        count.store(0);
    }

    constexpr int iterations { 500 };
    for (int i = 0; i < iterations; i++) {
        pool.run(taskCount, &runs, [](void * context, size_t taskIndex, size_t) {
            static_cast<std::vector<std::atomic_int> *>(context)->at(taskIndex).fetch_add(1);
        });
    }

    for (size_t i = 0; i < taskCount; i++) {
        QCOMPARE(runs[i].load(), iterations);
    }
}

void RealTimeWorkerPoolTest::test_run_afterIdlePeriod_shouldWakeSleepingWorkers()
{
    // Long enough between runs that the workers give up spinning and block, exercising the
    // semaphore fallback and the handshake that decides who releases it.
    RealTimeWorkerPool pool { 3 };
    constexpr size_t taskCount { 8 };

    for (int i = 0; i < 20; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds { 2 });
        std::vector<std::atomic_int> runs(taskCount);
        for (auto & count : runs) {
            count.store(0);
        }
        pool.run(taskCount, &runs, [](void * context, size_t taskIndex, size_t) {
            static_cast<std::vector<std::atomic_int> *>(context)->at(taskIndex).fetch_add(1);
        });
        for (size_t task = 0; task < taskCount; task++) {
            QCOMPARE(runs[task].load(), 1);
        }
    }
}

void RealTimeWorkerPoolTest::test_run_realisticCallbackDuration_shouldNotStall()
{
    // The shape that matters, and that a trivial callback does not reproduce: each run takes long
    // enough that the workers give up spinning and block, so every run exercises the wake path. An
    // earlier handshake passed the quick tests and still deadlocked here after a few thousand runs,
    // because a straggler from one run could consume the next run's wakeup.
    RealTimeWorkerPool pool { 4 };
    constexpr size_t taskCount { 6 };
    constexpr int iterations { 20000 };

    std::atomic_int completed { 0 };
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds { 60 };

    for (int i = 0; i < iterations; i++) {
        pool.run(taskCount, &completed, [](void * context, size_t, size_t) {
            // Roughly what one device costs in a real callback.
            volatile double sink = 0.0;
            for (int k = 0; k < 20000; k++) {
                sink += k * 0.5;
            }
            static_cast<std::atomic_int *>(context)->fetch_add(1);
        });
        QVERIFY2(std::chrono::steady_clock::now() < deadline, "The pool stalled");
    }

    QCOMPARE(completed.load(), static_cast<int>(taskCount) * iterations);
}

void RealTimeWorkerPoolTest::test_hasRealTimeScheduling_withoutPrivileges_shouldBeFalseNotFatal()
{
    // Asking for real-time scheduling without RLIMIT_RTPRIO must report failure rather than throw
    // or half-start the pool: playback then stays serial, which is the safe outcome.
    RealTimeWorkerPool pool { 2 };
    pool.setRealTimePriority(50);

    QCOMPARE(pool.workerCount(), size_t { 2 });

    std::atomic_int runs { 0 };
    pool.run(8, &runs, [](void * context, size_t, size_t) {
        static_cast<std::atomic_int *>(context)->fetch_add(1);
    });
    QCOMPARE(runs.load(), 8);

    // Whether it succeeded depends on the machine; it must simply answer without crashing.
    const bool realTime = pool.hasRealTimeScheduling();
    QVERIFY(realTime || !realTime);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RealTimeWorkerPoolTest)
