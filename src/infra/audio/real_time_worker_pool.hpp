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

#ifndef REAL_TIME_WORKER_POOL_HPP
#define REAL_TIME_WORKER_POOL_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <semaphore>
#include <thread>
#include <vector>

namespace noteahead {

class RealTimeWorkerPool
{
public:
    using TaskCallback = std::function<void(void * context, size_t taskIndex, size_t workerIndex)>;

    explicit RealTimeWorkerPool(size_t workerCount = defaultWorkerCount());
    ~RealTimeWorkerPool();

    RealTimeWorkerPool(const RealTimeWorkerPool &) = delete;
    RealTimeWorkerPool & operator=(const RealTimeWorkerPool &) = delete;

    size_t workerCount() const;
    size_t laneCount() const;

    void run(size_t taskCount, void * context, TaskCallback callback);

    static size_t defaultWorkerCount();

private:
    void workerLoop(size_t workerIndex);

    std::vector<std::thread> m_workers;
    std::vector<std::unique_ptr<std::binary_semaphore>> m_startSemaphores;
    std::binary_semaphore m_finished { 0 };

    std::atomic<bool> m_stopping { false };
    std::atomic<size_t> m_nextTask { 0 };
    std::atomic<size_t> m_activeWorkers { 0 };

    TaskCallback m_callback { nullptr };
    void * m_context { nullptr };
    size_t m_taskCount { 0 };
};

} // namespace noteahead

#endif // REAL_TIME_WORKER_POOL_HPP
