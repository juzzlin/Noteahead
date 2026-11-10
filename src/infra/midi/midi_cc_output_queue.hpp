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

#ifndef MIDI_CC_OUTPUT_QUEUE_HPP
#define MIDI_CC_OUTPUT_QUEUE_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <semaphore>
#include <string>
#include <thread>

namespace noteahead {

//! Carries MIDI CC messages from the audio thread, where a modulator aimed at external gear makes
//! them, to a thread of its own that hands them on to be sent.
//!
//! The audio thread only writes to a fixed ring and wakes the sender, so it never waits on MIDI. The
//! sender holds each controller to a maximum rate: of the values that come faster, the ones in
//! between are dropped, but never the last, which goes out as soon as the rate allows. A gesture
//! therefore always ends where it was left, and a slow port is never flooded.
class MidiCcOutputQueue
{
public:
    using Clock = std::chrono::steady_clock;
    //! Sends one message: the port's name, the channel 0 to 15, the controller and the value.
    using Sender = std::function<void(const std::string & port, uint8_t channel, uint8_t controller, uint8_t value)>;

    //! How many messages fit between two wakeups of the sender.
    static constexpr size_t Capacity { 1024 };
    //! How many ports it can tell apart. Far more than a studio has.
    static constexpr size_t MaxPorts { 64 };
    static constexpr int DefaultMaxRate { 100 };

    MidiCcOutputQueue();
    ~MidiCcOutputQueue();

    MidiCcOutputQueue(const MidiCcOutputQueue &) = delete;
    MidiCcOutputQueue & operator=(const MidiCcOutputQueue &) = delete;

    void setSender(Sender sender);
    //! Messages per second per controller, at least 1.
    void setMaxRate(int messagesPerSecond);
    int maxRate() const;

    //! Queues a message. For the audio thread: never blocks, and allocates only the first time it
    //! sees a port. False when the ring or the port table is full and the message was dropped.
    bool push(const std::string & port, uint8_t channel, uint8_t controller, uint8_t value);

private:
    struct Message
    {
        uint16_t port {};
        uint8_t channel {};
        uint8_t controller {};
        uint8_t value {};
    };

    std::optional<uint16_t> portIndex(const std::string & port);
    void run();

    std::array<std::string, MaxPorts> m_ports;
    //! Ports published to the sender. Written by the audio thread only.
    std::atomic<size_t> m_portCount { 0 };

    std::array<Message, Capacity> m_ring;
    std::atomic<size_t> m_head { 0 };
    std::atomic<size_t> m_tail { 0 };

    std::atomic<int> m_maxRate { DefaultMaxRate };
    std::atomic<bool> m_woken { false };
    std::atomic<bool> m_running { true };
    std::counting_semaphore<> m_wake { 0 };

    std::mutex m_senderMutex;
    Sender m_sender;
    std::thread m_thread;
};

} // namespace noteahead

#endif // MIDI_CC_OUTPUT_QUEUE_HPP
