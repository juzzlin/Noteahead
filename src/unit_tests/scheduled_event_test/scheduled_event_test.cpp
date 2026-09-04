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

#include "scheduled_event_test.hpp"

#include "../../domain/devices/device.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include <QTest>

#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate = 48000;

//! The smallest device that can show which frame a note started on: it writes the note number into
//! every frame it renders while a note is held, and silence otherwise. Whatever it fills the buffer
//! with is therefore a direct picture of when it was told.
class MarkerDevice : public Device
{
public:
    std::string name() const override
    {
        return "Marker";
    }

    std::string category() const override
    {
        return "Test";
    }

    std::string typeName() const override
    {
        return "Marker";
    }

    std::string typeId() const override
    {
        return "marker";
    }

    void processMidiNoteOn(uint8_t note, uint8_t) override
    {
        m_note = note;
    }

    void processMidiNoteOff(uint8_t) override
    {
        m_note = 0;
    }

    void processMidiCc(uint8_t controller, uint8_t value, uint8_t) override
    {
        m_ccLog.push_back({ controller, value });
    }

    void processMidiAllNotesOff() override
    {
        m_note = 0;
    }

    void processAudio(AudioContext & context) override
    {
        m_pieces.push_back({ context.startFrame, context.frameCount });
        for (uint32_t i = 0; i < context.frameCount; i++) {
            context.buffer[i * 2] = m_note;
            context.buffer[i * 2 + 1] = m_note;
        }
    }

    bool hasActiveAudio() const override
    {
        return m_note != 0;
    }

    void reset() override
    {
        m_note = 0;
    }

    void resetAudio() override
    {
        m_note = 0;
    }

    void serializeToXml(ProjectWriter &) const override
    {
    }

    void deserializeFromXml(ProjectReader &) override
    {
    }

    struct Piece
    {
        uint64_t startFrame {};
        uint32_t frameCount {};
    };

    const std::vector<Piece> & pieces() const
    {
        return m_pieces;
    }

    const std::vector<std::pair<uint8_t, uint8_t>> & ccLog() const
    {
        return m_ccLog;
    }

private:
    uint8_t m_note { 0 };
    std::vector<Piece> m_pieces;
    std::vector<std::pair<uint8_t, uint8_t>> m_ccLog;
};

Device::ScheduledEvent noteOnAt(uint64_t frame, uint8_t note)
{
    Device::ScheduledEvent event;
    event.type = Device::ScheduledEvent::Type::NoteOn;
    event.frame = frame;
    event.note = note;
    event.velocity = 100;
    return event;
}

//! Renders one block and hands back the left channel, one value per frame.
std::vector<double> renderBlock(MarkerDevice & device, uint64_t startFrame, uint32_t frameCount)
{
    std::vector<double> buffer(static_cast<size_t>(frameCount) * 2, 0.0);
    AudioContext context;
    context.buffer = std::span<double> { buffer };
    context.frameCount = frameCount;
    context.sampleRate = SampleRate;
    context.startFrame = startFrame;
    device.renderBlock(context);

    std::vector<double> left(frameCount);
    for (uint32_t i = 0; i < frameCount; i++) {
        left[i] = buffer[i * 2];
    }
    return left;
}

//! First frame carrying the given note.
int firstFrameOf(const std::vector<double> & left, uint8_t note)
{
    for (size_t i = 0; i < left.size(); i++) {
        if (static_cast<uint8_t>(left[i]) == note) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace

void ScheduledEventTest::test_renderBlock_withoutEvents_shouldRenderOnePiece()
{
    MarkerDevice device;
    renderBlock(device, 0, 512);

    QCOMPARE(device.pieces().size(), size_t { 1 });
    QCOMPARE(device.pieces().at(0).frameCount, uint32_t { 512 });
    QCOMPARE(device.pieces().at(0).startFrame, uint64_t { 0 });
}

void ScheduledEventTest::test_renderBlock_scheduledNote_shouldStartOnItsOwnFrame()
{
    MarkerDevice device;
    device.scheduleMidiEvent(noteOnAt(137, 60));

    const auto left = renderBlock(device, 0, 512);

    QCOMPARE(firstFrameOf(left, 60), 137);
    QCOMPARE(device.pieces().size(), size_t { 2 });
    QCOMPARE(device.pieces().at(0).frameCount, uint32_t { 137 });
    QCOMPARE(device.pieces().at(1).startFrame, uint64_t { 137 });
}

void ScheduledEventTest::test_renderBlock_severalEvents_shouldStartEachOnItsOwnFrame()
{
    MarkerDevice device;
    device.scheduleMidiEvent(noteOnAt(10, 60));
    device.scheduleMidiEvent(noteOnAt(200, 62));
    device.scheduleMidiEvent(noteOnAt(511, 64));

    const auto left = renderBlock(device, 0, 512);

    QCOMPARE(firstFrameOf(left, 60), 10);
    QCOMPARE(firstFrameOf(left, 62), 200);
    QCOMPARE(firstFrameOf(left, 64), 511);
    QCOMPARE(device.pieces().size(), size_t { 4 });
}

void ScheduledEventTest::test_renderBlock_eventInThePast_shouldStartAtTheBlockStart()
{
    // Nothing can be placed in audio that has already gone out, so an event whose frame has been
    // rendered past is played at the first frame left rather than dropped.
    MarkerDevice device;
    device.scheduleMidiEvent(noteOnAt(50, 60));

    const auto left = renderBlock(device, 512, 512);

    QCOMPARE(firstFrameOf(left, 60), 0);
    QCOMPARE(device.scheduledEventCount(), size_t { 0 });
}

void ScheduledEventTest::test_renderBlock_eventBeyondTheBlock_shouldWait()
{
    MarkerDevice device;
    device.scheduleMidiEvent(noteOnAt(600, 60));

    const auto left = renderBlock(device, 0, 512);

    QCOMPARE(firstFrameOf(left, 60), -1);
    QCOMPARE(device.pieces().size(), size_t { 1 });
    QCOMPARE(device.scheduledEventCount(), size_t { 1 });

    const auto next = renderBlock(device, 512, 512);
    QCOMPARE(firstFrameOf(next, 60), 88);
}

void ScheduledEventTest::test_renderBlock_shouldNotQuantiseToTheBlock_acrossABurst()
{
    // The case the whole thing exists for. Four 512-frame blocks are rendered back to back, the way
    // a server that hands over a burst of buffers asks for them, with notes falling at frames that
    // are nowhere near a block boundary. Every one has to keep its own frame.
    MarkerDevice device;
    constexpr uint32_t blockSize = 512;
    const std::vector<uint64_t> frames { 100, 700, 1300, 1900 };
    for (size_t i = 0; i < frames.size(); i++) {
        device.scheduleMidiEvent(noteOnAt(frames.at(i), static_cast<uint8_t>(60 + i)));
    }

    std::vector<double> stream;
    for (uint32_t block = 0; block < 4; block++) {
        const auto left = renderBlock(device, block * blockSize, blockSize);
        stream.insert(stream.end(), left.begin(), left.end());
    }

    for (size_t i = 0; i < frames.size(); i++) {
        QCOMPARE(firstFrameOf(stream, static_cast<uint8_t>(60 + i)), static_cast<int>(frames.at(i)));
    }
}

void ScheduledEventTest::test_scheduledEvents_shouldBeApplied_inTheOrderQueued()
{
    MarkerDevice device;
    for (uint8_t i = 0; i < 4; i++) {
        Device::ScheduledEvent event;
        event.type = Device::ScheduledEvent::Type::Cc;
        event.frame = 64;
        event.controller = 74;
        event.velocity = static_cast<uint8_t>(10 + i);
        device.scheduleMidiEvent(event);
    }

    renderBlock(device, 0, 512);

    QCOMPARE(device.ccLog().size(), size_t { 4 });
    for (uint8_t i = 0; i < 4; i++) {
        QCOMPARE(device.ccLog().at(i).second, static_cast<uint8_t>(10 + i));
    }
    // All at one frame, so the block is split once and not four times.
    QCOMPARE(device.pieces().size(), size_t { 2 });
}

void ScheduledEventTest::test_clearScheduledEvents_shouldDropEverythingQueued()
{
    MarkerDevice device;
    device.scheduleMidiEvent(noteOnAt(100, 60));
    device.scheduleMidiEvent(noteOnAt(200, 62));
    QCOMPARE(device.scheduledEventCount(), size_t { 2 });

    device.clearScheduledEvents();
    QCOMPARE(device.scheduledEventCount(), size_t { 0 });

    const auto left = renderBlock(device, 0, 512);
    QCOMPARE(firstFrameOf(left, 60), -1);
    QCOMPARE(device.pieces().size(), size_t { 1 });
}

void ScheduledEventTest::test_framesRendered_shouldAdvanceByEveryBlock()
{
    AudioEngine engine;
    const auto before = engine.framesRendered();

    std::vector<double> buffer(512 * 2, 0.0);
    for (uint32_t block = 0; block < 3; block++) {
        AudioContext context;
        context.buffer = std::span<double> { buffer };
        context.frameCount = 512;
        context.sampleRate = SampleRate;
        engine.process(context);
        // The engine stamps the block it is about to render, so the context comes back saying where
        // it started rather than where it ended.
        QCOMPARE(context.startFrame, before + block * 512);
    }

    QCOMPARE(engine.framesRendered(), before + 3 * 512);
}

void ScheduledEventTest::test_engine_scheduledNote_shouldStartOnItsOwnFrame()
{
    // The same thing end to end: the engine stamps the block, the device breaks it, and the note
    // comes out on the frame it was scheduled for rather than at the block boundary.
    AudioEngine engine;
    const auto device = std::make_shared<MarkerDevice>();
    engine.setDevice(0, device);

    const auto anchor = engine.framesRendered();
    device->scheduleMidiEvent(noteOnAt(anchor + 512 + 300, 60));

    std::vector<double> stream;
    std::vector<double> buffer(512 * 2, 0.0);
    for (uint32_t block = 0; block < 2; block++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context;
        context.buffer = std::span<double> { buffer };
        context.frameCount = 512;
        context.sampleRate = SampleRate;
        engine.process(context);
        for (uint32_t i = 0; i < 512; i++) {
            stream.push_back(buffer[i * 2]);
        }
    }

    QCOMPARE(firstFrameOf(stream, 60), 512 + 300);
}

void ScheduledEventTest::test_frameAnchor_beforeAnythingRendered_shouldSayItIsNotRunning()
{
    // Nothing can be placed in a stream that is not moving, and whoever schedules has to be able to
    // tell, so that it can fall back to the clock on the wall instead of stalling.
    const AudioEngine engine;
    QVERIFY(!engine.frameAnchor().running);
}

void ScheduledEventTest::test_frameAnchor_shouldTurnATimeIntoAFrame()
{
    AudioEngine engine;
    std::vector<double> buffer(512 * 2, 0.0);

    const auto renderOne = [&] {
        AudioContext context;
        context.buffer = std::span<double> { buffer };
        context.frameCount = 512;
        context.sampleRate = SampleRate;
        engine.process(context);
    };

    renderOne();
    const auto first = engine.frameAnchor();
    QVERIFY(first.running);
    QCOMPARE(first.sampleRate, SampleRate);

    renderOne();
    const auto second = engine.frameAnchor();

    // One block on, so the anchor has moved a block's worth of frames and a block's worth of time.
    QCOMPARE(second.frame - first.frame, uint64_t { 512 });
    QVERIFY(second.nanoseconds >= first.nanoseconds);

    // What the player will do with it: a time turned into the frame it falls on.
    const auto frameAt = [&](int64_t nanoseconds) {
        const auto anchor = engine.frameAnchor();
        const double seconds = static_cast<double>(nanoseconds - anchor.nanoseconds) / 1e9;
        return anchor.frame + static_cast<int64_t>(seconds * anchor.sampleRate);
    };
    QCOMPARE(frameAt(second.nanoseconds), static_cast<int64_t>(second.frame));
    QCOMPARE(frameAt(second.nanoseconds + 1000000000LL / 100), static_cast<int64_t>(second.frame) + SampleRate / 100);
}

void ScheduledEventTest::test_renderBlock_ccOnANotesFrame_shouldArriveWithTheNote()
{
    // Written on the same line, so they have to reach the device together. What makes this worth a
    // test is the failure it guards: notes running ahead while controller moves stayed immediate
    // would put the value a whole lookahead in front of the note it was written for.
    MarkerDevice device;

    Device::ScheduledEvent cc;
    cc.type = Device::ScheduledEvent::Type::Cc;
    cc.frame = 300;
    cc.controller = 74;
    cc.velocity = 99;
    device.scheduleMidiEvent(cc);
    device.scheduleMidiEvent(noteOnAt(300, 60));

    // A block that ends before them leaves both waiting, rather than letting one through.
    renderBlock(device, 0, 256);
    QCOMPARE(device.ccLog().size(), size_t { 0 });
    QCOMPARE(device.scheduledEventCount(), size_t { 2 });

    const auto left = renderBlock(device, 256, 256);

    QCOMPARE(device.ccLog().size(), size_t { 1 });
    QCOMPARE(device.ccLog().at(0).first, uint8_t { 74 });
    QCOMPARE(device.ccLog().at(0).second, uint8_t { 99 });
    QCOMPARE(firstFrameOf(left, 60), 300 - 256);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ScheduledEventTest)
