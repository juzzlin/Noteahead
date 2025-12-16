// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <atomic>
#include <cstddef>
#include <vector>
#include <algorithm>

namespace noteahead {

template <typename T>
class RingBuffer
{
public:
    explicit RingBuffer(size_t size = 0)
    {
        if (size > 0) {
            resize(size);
        }
    }

    void resize(size_t size)
    {
        m_buffer.resize(size);
        m_head.store(0);
        m_tail.store(0);
    }

    // Push data into the buffer. Returns false if not enough space.
    // This should be called by the producer (audio callback).
    bool push(const T * data, size_t count)
    {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t tail = m_tail.load(std::memory_order_acquire);
        const size_t capacity = m_buffer.size();

        if (capacity == 0) return false;

        // Calculate available space
        // One slot is kept empty to distinguish full from empty
        const size_t available = (capacity + tail - head - 1) % capacity;

        if (available < count) {
            return false;
        }

        // Copy data
        for (size_t i = 0; i < count; ++i) {
            m_buffer[(head + i) % capacity] = data[i];
        }

        m_head.store((head + count) % capacity, std::memory_order_release);
        return true;
    }

    // Pop data from the buffer. Returns number of elements read.
    // This should be called by the consumer (writer thread).
    size_t pop(T * data, size_t count)
    {
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        const size_t head = m_head.load(std::memory_order_acquire);
        const size_t capacity = m_buffer.size();

        if (capacity == 0) return 0;

        const size_t available = (capacity + head - tail) % capacity;

        if (available == 0) {
            return 0;
        }

        const size_t toRead = std::min(available, count);

        for (size_t i = 0; i < toRead; ++i) {
            data[i] = m_buffer[(tail + i) % capacity];
        }

        m_tail.store((tail + toRead) % capacity, std::memory_order_release);

        return toRead;
    }

    size_t readAvailable() const
    {
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        const size_t head = m_head.load(std::memory_order_acquire);
        const size_t capacity = m_buffer.size();
        if (capacity == 0) return 0;
        return (capacity + head - tail) % capacity;
    }

private:
    std::vector<T> m_buffer;
    std::atomic<size_t> m_head { 0 };
    std::atomic<size_t> m_tail { 0 };
};

} // namespace noteahead

#endif // RING_BUFFER_HPP
