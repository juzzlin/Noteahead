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
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace noteahead {

//! Fan-out/fan-in worker pool for the audio engine, usable from a real-time callback.
//!
//! Two things make that possible, and both matter — an earlier attempt at threading playback had
//! neither and had to be reverted for stutter:
//!
//! 1. The workers can run at real-time priority. A pool of normal-priority threads that a real-time
//!    callback waits on is a priority inversion: the moment the scheduler preempts a worker, the
//!    callback misses its deadline. Playback must therefore only fan out when
//!    hasRealTimeScheduling() is true.
//! 2. The handshake spins before it blocks. Waking threads costs a syscall and scheduler latency
//!    each way, which at a couple of hundred buffers a second dominates work that only takes a few
//!    hundred microseconds. In the steady state no syscall happens at all.
//!
//! Both waits block on the atomic they are actually interested in, via atomic::wait, rather than
//! passing wakeup tokens through semaphores. That is deliberate: a token can be released by a
//! straggler from the previous run and consumed by the next one, which desynchronises the fan-in
//! and eventually hangs the caller. atomic::wait re-checks the value itself, so there is no token
//! to lose or to misattribute.
class RealTimeWorkerPool
{
public:
    using TaskCallback = std::function<void(void * context, size_t taskIndex, size_t workerIndex)>;

    //! Starts one worker thread. Returns false if the thread could not be created, and sets
    //! realTime to whether it got real-time scheduling.
    using ThreadFactory = std::function<bool(size_t index, std::function<void()> body, bool & realTime)>;

    explicit RealTimeWorkerPool(size_t workerCount = defaultWorkerCount());
    ~RealTimeWorkerPool();

    RealTimeWorkerPool(const RealTimeWorkerPool &) = delete;
    RealTimeWorkerPool & operator=(const RealTimeWorkerPool &) = delete;

    size_t workerCount() const;
    size_t laneCount() const;

    void run(size_t taskCount, void * context, TaskCallback callback);

    //! Restarts the workers using the given factory, so a backend can create them the way it must —
    //! JACK hands out real-time priority through its own call, for instance. Must not be called
    //! while run() is in flight.
    void setThreadFactory(ThreadFactory factory);

    //! True only when *every* worker got real-time scheduling. Playback must not fan out otherwise.
    bool hasRealTimeScheduling() const;

    //! Priority the default factory asks for, when positive. Ignored by a custom factory.
    void setRealTimePriority(int priority);

    static size_t defaultWorkerCount();

private:
    void startWorkers();
    void stopWorkers();
    void workerLoop(size_t workerIndex);
    //! Blocks until the generation counter moves past lastGeneration. Returns false when stopping.
    bool waitForWork(uint64_t & lastGeneration);
    //! Blocks until every enlisted worker has finished this run.
    void waitForWorkers();

    //! Spins before blocking. A few microseconds on a modern core, which covers the gap between
    //! the callback starting a run and a worker noticing it.
    static constexpr int SpinCount { 2000 };

    std::vector<std::thread> m_workers;
    std::vector<uint8_t> m_workerIsRealTime;

    size_t m_requestedWorkerCount { 0 };
    ThreadFactory m_threadFactory {};
    int m_realTimePriority { 0 };

    std::atomic_bool m_stopping { false };
    std::atomic<uint64_t> m_generation { 0 };
    std::atomic<size_t> m_nextTask { 0 };
    //! Counts down as workers finish. Every worker decrements exactly once per generation, so the
    //! count cannot be left inconsistent by a straggler crossing into the next run.
    std::atomic<size_t> m_activeWorkers { 0 };

    TaskCallback m_callback { nullptr };
    void * m_context { nullptr };
    size_t m_taskCount { 0 };
};

} // namespace noteahead

#endif // REAL_TIME_WORKER_POOL_HPP
