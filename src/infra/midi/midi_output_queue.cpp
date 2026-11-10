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

#include "midi_output_queue.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <tuple>

namespace noteahead {

MidiOutputQueue::MidiOutputQueue()
  : m_thread { [this] { run(); } }
{
}

MidiOutputQueue::~MidiOutputQueue()
{
    m_running = false;
    m_wake.release();
    m_thread.join();
}

void MidiOutputQueue::setSender(Sender sender)
{
    const std::lock_guard<std::mutex> lock { m_senderMutex };
    m_sender = std::move(sender);
}

void MidiOutputQueue::setMaxRate(int messagesPerSecond)
{
    m_maxRate = std::max(1, messagesPerSecond);
    // A lower rate shortens nothing already waiting, a higher one may let it out now
    m_wake.release();
}

int MidiOutputQueue::maxRate() const
{
    return m_maxRate;
}

std::optional<uint16_t> MidiOutputQueue::portIndex(const std::string & port)
{
    const size_t count = m_portCount.load(std::memory_order_relaxed);
    for (size_t index = 0; index < count; index++) {
        if (m_ports.at(index) == port) {
            return static_cast<uint16_t>(index);
        }
    }
    if (count == MaxPorts) {
        return std::nullopt;
    }
    m_ports.at(count) = port;
    m_portCount.store(count + 1, std::memory_order_release);
    return static_cast<uint16_t>(count);
}

bool MidiOutputQueue::push(const std::string & port, uint8_t channel, std::optional<uint8_t> controller, double value)
{
    const auto index = portIndex(port);
    if (!index) {
        return false;
    }
    const size_t head = m_head.load(std::memory_order_relaxed);
    const size_t next = (head + 1) % Capacity;
    if (next == m_tail.load(std::memory_order_acquire)) {
        return false;
    }
    m_ring.at(head) = { *index, channel, controller, value };
    m_head.store(next, std::memory_order_release);
    // One wakeup is enough for any number of messages, and keeps the semaphore's count from growing
    if (!m_woken.exchange(true, std::memory_order_acq_rel)) {
        m_wake.release();
    }
    return true;
}

void MidiOutputQueue::run()
{
    struct Pending
    {
        double value {};
        bool waiting {};
        Clock::time_point nextAllowed {};
    };

    // By port, channel and controller, or no controller for the channel's pitch bend
    std::map<std::tuple<uint16_t, uint8_t, std::optional<uint8_t>>, Pending> controllers;

    while (true) {
        std::optional<Clock::time_point> deadline;
        for (auto && [key, pending] : controllers) {
            if (pending.waiting && (!deadline || pending.nextAllowed < *deadline)) {
                deadline = pending.nextAllowed;
            }
        }
        if (deadline) {
            [[maybe_unused]] const bool woken = m_wake.try_acquire_until(*deadline);
        } else {
            m_wake.acquire();
        }
        m_woken.store(false, std::memory_order_release);
        if (!m_running) {
            return;
        }

        // Only the latest value of each controller and pitch bend matters
        for (size_t tail = m_tail.load(std::memory_order_relaxed); tail != m_head.load(std::memory_order_acquire); tail = (tail + 1) % Capacity) {
            const auto message = m_ring.at(tail);
            auto & pending = controllers[{ message.port, message.channel, message.controller }];
            pending.value = message.value;
            pending.waiting = true;
            m_tail.store((tail + 1) % Capacity, std::memory_order_release);
        }

        const auto now = Clock::now();
        const auto interval = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / static_cast<double>(m_maxRate.load())));
        const size_t portCount = m_portCount.load(std::memory_order_acquire);
        const std::lock_guard<std::mutex> lock { m_senderMutex };
        for (auto && [key, pending] : controllers) {
            if (pending.waiting && pending.nextAllowed <= now) {
                const auto & [port, channel, controller] = key;
                if (m_sender && port < portCount) {
                    m_sender(m_ports.at(port), channel, controller, pending.value);
                }
                pending.waiting = false;
                pending.nextAllowed = now + interval;
            }
        }
    }
}

} // namespace noteahead
