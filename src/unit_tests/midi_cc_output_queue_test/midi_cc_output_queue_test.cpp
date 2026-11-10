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

#include "midi_cc_output_queue_test.hpp"

#include "../../infra/midi/midi_cc_output_queue.hpp"

#include <QTest>

#include <mutex>
#include <tuple>
#include <vector>

namespace noteahead {

namespace {

//! What the queue handed on, as it arrived
class Received
{
public:
    explicit Received(MidiCcOutputQueue & queue)
    {
        queue.setSender([this](const std::string & port, uint8_t channel, uint8_t controller, uint8_t value) {
            const std::lock_guard<std::mutex> lock { m_mutex };
            m_messages.emplace_back(port, channel, controller, value);
        });
    }

    using Message = std::tuple<std::string, uint8_t, uint8_t, uint8_t>;

    std::vector<Message> messages() const
    {
        const std::lock_guard<std::mutex> lock { m_mutex };
        return m_messages;
    }

    std::vector<uint8_t> values(uint8_t controller) const
    {
        std::vector<uint8_t> values;
        for (auto && message : messages()) {
            if (std::get<2>(message) == controller) {
                values.push_back(std::get<3>(message));
            }
        }
        return values;
    }

private:
    mutable std::mutex m_mutex;
    std::vector<Message> m_messages;
};

} // namespace

void MidiCcOutputQueueTest::test_push_shouldReachTheSender()
{
    MidiCcOutputQueue queue;
    Received received { queue };
    QVERIFY(queue.push("Port A", 2, 74, 100));
    QTRY_COMPARE(received.messages().size(), size_t { 1 });
    QCOMPARE(received.messages().front(), (Received::Message { "Port A", 2, 74, 100 }));
}

void MidiCcOutputQueueTest::test_push_fasterThanTheRate_shouldDropTheValuesBetweenButNeverTheLast()
{
    MidiCcOutputQueue queue;
    Received received { queue };
    queue.setMaxRate(10);
    for (uint8_t value = 0; value <= 100; value++) {
        QVERIFY(queue.push("Port", 0, 1, value));
    }
    QTRY_VERIFY(!received.values(1).empty() && received.values(1).back() == 100);
    // A hundred values in no time, at ten a second: the first and the last
    QVERIFY(received.values(1).size() <= 3);
}

void MidiCcOutputQueueTest::test_push_controllers_shouldEachHaveARateOfTheirOwn()
{
    MidiCcOutputQueue queue;
    Received received { queue };
    queue.setMaxRate(10);
    QVERIFY(queue.push("Port", 0, 1, 10));
    QVERIFY(queue.push("Port", 0, 74, 20));
    QVERIFY(queue.push("Other", 0, 1, 30));
    QTRY_COMPARE(received.messages().size(), size_t { 3 });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiCcOutputQueueTest)
