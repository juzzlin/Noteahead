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
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <algorithm>
#include <pthread.h>

namespace noteahead {

RealTimeWorkerPool::RealTimeWorkerPool(size_t workerCount)
{
    m_startSemaphores.reserve(workerCount);
    m_workers.reserve(workerCount);

    for (size_t i = 0; i < workerCount; ++i) {
        m_startSemaphores.emplace_back(std::make_unique<std::binary_semaphore>(0));
    }

    for (size_t i = 0; i < workerCount; ++i) {
        m_workers.emplace_back([this, i] {
            // Set thread name for easier debugging
            const std::string threadName = "AudioWorker-" + std::to_string(i);
            pthread_setname_np(pthread_self(), threadName.c_str());

            // Set real-time priority
            struct sched_param param;
            param.sched_priority = 80; // High priority for audio
            if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
                juzzlin::L("RealTimeWorkerPool").warning() << "Failed to set RT priority for " << threadName;
            }

            workerLoop(i);
        });
    }
}

RealTimeWorkerPool::~RealTimeWorkerPool()
{
    m_stopping.store(true, std::memory_order_release);
    for (auto & semaphore : m_startSemaphores) {
        semaphore->release();
    }
    for (auto & worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t RealTimeWorkerPool::workerCount() const
{
    return m_workers.size();
}

size_t RealTimeWorkerPool::laneCount() const
{
    return m_workers.size() + 1;
}

void RealTimeWorkerPool::run(size_t taskCount, void * context, TaskCallback callback)
{
    if (taskCount == 0 || callback == nullptr) {
        return;
    }

    if (m_workers.empty() || taskCount == 1) {
        for (size_t taskIndex = 0; taskIndex < taskCount; ++taskIndex) {
            callback(context, taskIndex, 0);
        }
        return;
    }

    m_context = context;
    m_callback = callback;
    m_taskCount = taskCount;
    const size_t mainWorkerIndex = m_workers.size();
    m_nextTask.store(1, std::memory_order_relaxed);
    const size_t workersToUse = std::min(m_workers.size(), taskCount - 1);
    m_activeWorkers.store(workersToUse, std::memory_order_release);

    for (size_t workerIndex = 0; workerIndex < workersToUse; ++workerIndex) {
        m_startSemaphores[workerIndex]->release();
    }

    callback(context, 0, mainWorkerIndex);

    while (true) {
        const size_t taskIndex = m_nextTask.fetch_add(1, std::memory_order_relaxed);
        if (taskIndex >= m_taskCount) {
            break;
        }
        callback(context, taskIndex, mainWorkerIndex);
    }

    if (workersToUse > 0) {
        m_finished.acquire();
    }
}

size_t RealTimeWorkerPool::defaultWorkerCount()
{
    const auto hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads <= 1) {
        return 0;
    }
    return std::min<size_t>(3, static_cast<size_t>(hardwareThreads - 1));
}

void RealTimeWorkerPool::workerLoop(size_t workerIndex)
{
    while (true) {
        m_startSemaphores[workerIndex]->acquire();
        if (m_stopping.load(std::memory_order_acquire)) {
            break;
        }

        while (true) {
            const size_t taskIndex = m_nextTask.fetch_add(1, std::memory_order_relaxed);
            if (taskIndex >= m_taskCount) {
                break;
            }
            m_callback(m_context, taskIndex, workerIndex);
        }

        if (m_activeWorkers.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            m_finished.release();
        }
    }
}

} // namespace noteahead
