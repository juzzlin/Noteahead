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

#include "ring_buffer_test.hpp"
#include "../../infra/audio/ring_buffer.hpp"

#include <QtTest>
#include <vector>

namespace noteahead {

void RingBufferTest::test_pushPop_basic()
{
    RingBuffer<int> rb(10);
    const std::vector<int> input = { 1, 2, 3, 4, 5 };
    
    QVERIFY(rb.push(input.data(), input.size()));
    QCOMPARE(rb.readAvailable(), 5u);
    
    std::vector<int> output(5);
    QCOMPARE(rb.pop(output.data(), 5), 5u);
    QCOMPARE(rb.readAvailable(), 0u);
    
    for (size_t i = 0; i < input.size(); ++i) {
        QCOMPARE(output[i], input[i]);
    }
}

void RingBufferTest::test_capacity_and_available()
{
    // Note: RingBuffer keeps one slot empty to distinguish full from empty
    RingBuffer<int> rb(5);
    QCOMPARE(rb.writeAvailable(), 4u);
    QCOMPARE(rb.readAvailable(), 0u);
    
    int val = 42;
    rb.push(&val, 1);
    QCOMPARE(rb.writeAvailable(), 3u);
    QCOMPARE(rb.readAvailable(), 1u);
}

void RingBufferTest::test_wrapping()
{
    RingBuffer<int> rb(5); // Internal capacity is 5, usable is 4
    
    std::vector<int> data = { 1, 2, 3 };
    rb.push(data.data(), 3);
    
    std::vector<int> dummy(2);
    rb.pop(dummy.data(), 2); // tail is now at 2, head at 3
    
    // Now push 2 more items, this should wrap around the end
    std::vector<int> wrapData = { 4, 5 };
    QVERIFY(rb.push(wrapData.data(), 2));
    
    std::vector<int> finalOutput(3);
    QCOMPARE(rb.pop(finalOutput.data(), 3), 3u);
    
    QCOMPARE(finalOutput[0], 3);
    QCOMPARE(finalOutput[1], 4);
    QCOMPARE(finalOutput[2], 5);
}

void RingBufferTest::test_overflow()
{
    RingBuffer<int> rb(5); // usable 4
    std::vector<int> data = { 1, 2, 3, 4, 5 };
    
    QVERIFY(!rb.push(data.data(), 5));
    QVERIFY(rb.push(data.data(), 4));
}

void RingBufferTest::test_clear()
{
    RingBuffer<int> rb(10);
    int data[3] = { 1, 2, 3 };
    rb.push(data, 3);
    QCOMPARE(rb.readAvailable(), 3u);
    
    rb.clear();
    QCOMPARE(rb.readAvailable(), 0u);
    QCOMPARE(rb.writeAvailable(), 9u);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RingBufferTest)
