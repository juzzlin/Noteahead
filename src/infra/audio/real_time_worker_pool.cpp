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

#include "real_time_worker_pool.hpp"
#include "../../common/denormal_protection.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <algorithm>
#include <cstdlib>
#include <pthread.h>

namespace noteahead {

namespace {

constexpr auto TAG = "RealTimeWorkerPool";

//! Hint to the CPU that this is a spin loop, so it does not burn a full issue slot per iteration.
void spinPause()
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    asm volatile("yield" ::: "memory");
#endif
}

//! Puts a thread on SCHED_FIFO. Fails without RLIMIT_RTPRIO, which a stock desktop often lacks —
//! the caller then has to keep playback serial rather than risk the priority inversion.
//!
//! Done from the spawning thread rather than inside the worker so the answer is known before the
//! pool is used, instead of racing with the worker starting up.
bool tryRealTimeScheduling(pthread_t thread, int priority)
{
    if (priority <= 0) {
        return false;
    }
    const int lowest = sched_get_priority_min(SCHED_FIFO);
    const int highest = sched_get_priority_max(SCHED_FIFO);
    sched_param parameters {};
    parameters.sched_priority = std::clamp(priority, lowest, highest);
    return pthread_setschedparam(thread, SCHED_FIFO, &parameters) == 0;
}

} // namespace

RealTimeWorkerPool::RealTimeWorkerPool(size_t workerCount)
  : m_requestedWorkerCount { workerCount }
{
    juzzlin::L(TAG).info() << "Worker pool: " << workerCount << " worker thread(s)"
                           << (workerCount == 0 ? " (single-threaded)" : "");
    startWorkers();
}

RealTimeWorkerPool::~RealTimeWorkerPool()
{
    stopWorkers();
}

void RealTimeWorkerPool::startWorkers()
{
    const size_t workerCount = m_requestedWorkerCount;
    m_stopping.store(false, std::memory_order_release);
    m_generation.store(0, std::memory_order_release);

    m_workers.clear();
    m_workerIsRealTime.assign(workerCount, 0);
    m_workers.reserve(workerCount);
    for (size_t i = 0; i < workerCount; i++) {
        auto body = [this, i] {
            enableHardwareDenormalProtection();
            pthread_setname_np(pthread_self(), ("AudioWorker-" + std::to_string(i)).c_str());
            workerLoop(i);
        };

        bool realTime = false;
        if (m_threadFactory) {
            if (!m_threadFactory(i, body, realTime)) {
                juzzlin::L(TAG).error() << "Worker thread " << i << " could not be started";
                continue;
            }
        } else {
            m_workers.emplace_back(body);
            realTime = tryRealTimeScheduling(m_workers.back().native_handle(), m_realTimePriority);
        }
        m_workerIsRealTime[i] = realTime ? 1 : 0;
    }
}

void RealTimeWorkerPool::stopWorkers()
{
    m_stopping.store(true, std::memory_order_release);
    m_generation.fetch_add(1, std::memory_order_release);
    m_generation.notify_all();
    for (auto & worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
}

void RealTimeWorkerPool::setThreadFactory(ThreadFactory factory)
{
    stopWorkers();
    m_threadFactory = std::move(factory);
    startWorkers();
}

void RealTimeWorkerPool::setRealTimePriority(int priority)
{
    if (m_realTimePriority == priority) {
        return;
    }
    stopWorkers();
    m_realTimePriority = priority;
    startWorkers();
}

bool RealTimeWorkerPool::hasRealTimeScheduling() const
{
    if (m_workerIsRealTime.empty()) {
        return false;
    }
    return std::ranges::all_of(m_workerIsRealTime, [](uint8_t value) { return value != 0; });
}

size_t RealTimeWorkerPool::workerCount() const
{
    return m_workers.size();
}

size_t RealTimeWorkerPool::laneCount() const
{
    return m_workers.size() + 1;
}

bool RealTimeWorkerPool::waitForWork(uint64_t & lastGeneration)
{
    for (int spin = 0; spin < SpinCount; spin++) {
        if (m_generation.load(std::memory_order_acquire) != lastGeneration) {
            break;
        }
        spinPause();
    }

    // Blocking on the generation counter itself: wait() re-checks the value, so a wakeup can be
    // neither lost nor consumed by the wrong run.
    while (true) {
        const uint64_t generation = m_generation.load(std::memory_order_acquire);
        if (generation != lastGeneration) {
            lastGeneration = generation;
            break;
        }
        m_generation.wait(generation, std::memory_order_acquire);
    }

    return !m_stopping.load(std::memory_order_acquire);
}

void RealTimeWorkerPool::waitForWorkers()
{
    for (int spin = 0; spin < SpinCount; spin++) {
        if (!m_activeWorkers.load(std::memory_order_acquire)) {
            return;
        }
        spinPause();
    }

    while (true) {
        const size_t remaining = m_activeWorkers.load(std::memory_order_acquire);
        if (!remaining) {
            return;
        }
        m_activeWorkers.wait(remaining, std::memory_order_acquire);
    }
}

void RealTimeWorkerPool::workerLoop(size_t workerIndex)
{
    uint64_t lastGeneration = 0;
    while (waitForWork(lastGeneration)) {
        while (true) {
            const size_t taskIndex = m_nextTask.fetch_add(1, std::memory_order_relaxed);
            if (taskIndex >= m_taskCount) {
                break;
            }
            m_callback(m_context, taskIndex, workerIndex);
        }

        if (m_activeWorkers.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            m_activeWorkers.notify_all();
        }
    }
}

void RealTimeWorkerPool::run(size_t taskCount, void * context, TaskCallback callback)
{
    if (taskCount == 0 || callback == nullptr) {
        return;
    }

    if (m_workers.empty() || taskCount == 1) {
        for (size_t taskIndex = 0; taskIndex < taskCount; taskIndex++) {
            callback(context, taskIndex, 0);
        }
        return;
    }

    m_context = context;
    m_callback = callback;
    m_taskCount = taskCount;
    const size_t mainWorkerIndex = m_workers.size();
    m_nextTask.store(1, std::memory_order_relaxed);
    // Every worker wakes on the generation bump and decrements exactly once, whether or not it
    // found any work. Enlisting only some of them would let a straggler from this run decrement the
    // next run's count, which underflows it and hangs the caller — on the audio thread, fatally.
    m_activeWorkers.store(m_workers.size(), std::memory_order_release);

    m_generation.fetch_add(1, std::memory_order_release);
    m_generation.notify_all();

    callback(context, 0, mainWorkerIndex);

    while (true) {
        const size_t taskIndex = m_nextTask.fetch_add(1, std::memory_order_relaxed);
        if (taskIndex >= m_taskCount) {
            break;
        }
        callback(context, taskIndex, mainWorkerIndex);
    }

    waitForWorkers();
}

size_t RealTimeWorkerPool::defaultWorkerCount()
{
    const auto hardwareThreads = std::thread::hardware_concurrency();

    // NOTEAHEAD_AUDIO_WORKERS overrides the worker count (0 = single-threaded).
    if (const char * env = std::getenv("NOTEAHEAD_AUDIO_WORKERS"); env && *env) {
        char * end = nullptr;
        const long requested = std::strtol(env, &end, 10);
        if (end != env && requested >= 0) {
            const auto clamped = std::min<long>(requested, hardwareThreads > 0 ? hardwareThreads : requested);
            juzzlin::L(TAG).info() << "Worker count overridden via NOTEAHEAD_AUDIO_WORKERS: " << clamped;
            return static_cast<size_t>(clamped);
        }
        juzzlin::L(TAG).warning() << "Ignoring invalid NOTEAHEAD_AUDIO_WORKERS value: " << env;
    }

    if (hardwareThreads <= 1) {
        return 0;
    }
    if (hardwareThreads <= 4) {
        return static_cast<size_t>(hardwareThreads - 1);
    }
    // Leave at least 2 logical threads for the system/UI on higher core counts
    return static_cast<size_t>(hardwareThreads - 2);
}

} // namespace noteahead
