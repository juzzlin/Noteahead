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

#include "player_worker_test.hpp"

#include "../../application/service/dummy_midi_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/player_worker.hpp"
#include "../../common/constants.hpp"
#include "../../domain/midi/midi_note_data.hpp"
#include "../../domain/tracker/event.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"

#include <QTest>

#include <atomic>
#include <memory>
#include <thread>

namespace noteahead {

// Subclass to access protected members
class TestablePlayerWorker : public PlayerWorker
{
public:
    using PlayerWorker::PlayerWorker;

    void test_handleEvent(const Event & event)
    {
        handleEvent(event);
    }

    void callCheckMixerState()
    {
        checkMixerState();
    }

    std::chrono::steady_clock::duration lookahead() const
    {
        return scheduleLookahead();
    }

    void callResolveLookahead()
    {
        resolveScheduleLookahead();
    }
};

// Mock MidiService to capture calls
class MockMidiService : public DummyMidiService
{
public:
    MockMidiService()
      : DummyMidiService { nullptr }
    {
    }

    int stopAllNotesCallCount = 0;
    int playNoteCallCount = 0;
    int stopNoteCallCount = 0;

    void stopAllNotes(InstrumentW) override
    {
        stopAllNotesCallCount++;
    }

    int releaseAllNotesCallCount = 0;

    void releaseAllNotes(InstrumentW) override
    {
        releaseAllNotesCallCount++;
    }

    std::optional<double> lastNoteBeats;

    void playNote(InstrumentW, MidiNoteDataCR data) override
    {
        playNoteCallCount++;
        lastNoteBeats = data.noteBeats();
    }

    void stopNote(InstrumentW, MidiNoteDataCR) override
    {
        stopNoteCallCount++;
    }

    bool internal = false;

    bool isInternalInstrument(InstrumentW) const override
    {
        return internal;
    }

    std::optional<std::chrono::steady_clock::duration> lookahead;

    std::optional<std::chrono::steady_clock::duration> scheduleLookahead() const override
    {
        return lookahead;
    }
};

namespace {

struct WorkerContext
{
    std::shared_ptr<MockMidiService> midiService;
    std::shared_ptr<MixerService> mixerService;
    TestablePlayerWorker worker;
};

WorkerContext createWorkerContext()
{
    const auto midiService = std::make_shared<MockMidiService>();
    const auto mixerService = std::make_shared<MixerService>();
    return WorkerContext {
        midiService,
        mixerService,
        TestablePlayerWorker { midiService, mixerService, nullptr }
    };
}

std::shared_ptr<Instrument> makeInstrument(const QString & portName = "TestPort")
{
    const auto instrument = std::make_shared<Instrument>(portName);
    instrument->setMidiAddress(MidiAddress { portName, 0 });
    return instrument;
}

std::shared_ptr<Event> makeNoteOnEvent(const std::shared_ptr<Instrument> & instrument, size_t track = 0, size_t column = 0, uint8_t note = 60, uint8_t velocity = 100)
{
    NoteData noteData { track, column };
    noteData.setAsNoteOn(note, velocity);
    const auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    return event;
}

} // namespace

void PlayerWorkerTest::test_mixerChange_shouldStopNotes()
{
    auto context = createWorkerContext();

    // Configure Mixer: Track 0 enabled
    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0 });

    const auto instrument = makeInstrument();
    const auto event = makeNoteOnEvent(instrument);

    PlayerWorker::EventList events { event };
    PlayerWorker::Timing timing { 120, 4, 6 };

    context.worker.initialize(events, timing);

    // Simulate playback of the note to add it to m_activeNotes
    context.worker.test_handleEvent(*event);

    // Act: Mute Track 0
    context.mixerService->muteTrack(0, true);

    // Simulate mixer change handling
    context.worker.callCheckMixerState();

    // Assert: stopNote should be called for the active note on the muted track
    QCOMPARE(context.midiService->stopNoteCallCount, 1);
    QCOMPARE(context.midiService->stopAllNotesCallCount, 0);
}

//! A worker with one note-on on track 0, ready to be handed to test_handleEvent().
PlayerWorker::EventList makeNoteOnEvents(const std::shared_ptr<Event> & event, MixerService & mixerService)
{
    mixerService.setTrackIndices({ 0 });
    mixerService.setColumnIndices(0, { 0 });
    return { event };
}

void PlayerWorkerTest::test_noteOn_shouldCarryTheNoteLengthInBeats()
{
    // A device that has to fit something inside the note -- a spoken line -- needs the length when
    // the note starts. In beats rather than seconds, so that a tempo change while it sounds moves
    // what was fitted inside it along with everything else.
    auto context = createWorkerContext();

    const auto instrument = makeInstrument();
    const auto event = makeNoteOnEvent(instrument);
    // Four lines of six ticks is a beat, so two beats is forty-eight ticks.
    event->setNoteOffTick(48);

    const PlayerWorker::Timing timing { 120, 4, 6 };
    context.worker.initialize(makeNoteOnEvents(event, *context.mixerService), timing);
    context.worker.test_handleEvent(*event);

    QCOMPARE(context.midiService->playNoteCallCount, 1);
    QVERIFY(context.midiService->lastNoteBeats.has_value());
    QVERIFY(std::abs(*context.midiService->lastNoteBeats - 2.0) < 1.0e-9);
}

void PlayerWorkerTest::test_noteOn_withoutANoteOffTick_shouldCarryNoLength()
{
    // Nothing says how long it lasts, so nothing is claimed. What reads the length falls back to
    // whatever it would have done without one.
    auto context = createWorkerContext();

    const auto instrument = makeInstrument();
    const auto event = makeNoteOnEvent(instrument);

    const PlayerWorker::Timing timing { 120, 4, 6 };
    context.worker.initialize(makeNoteOnEvents(event, *context.mixerService), timing);
    context.worker.test_handleEvent(*event);

    QCOMPARE(context.midiService->playNoteCallCount, 1);
    QVERIFY(!context.midiService->lastNoteBeats.has_value());
}

void PlayerWorkerTest::test_columnMuteBehavior_shouldNotStopAllNotes()
{
    auto context = createWorkerContext();

    // Configure Mixer: Track 0 has 3 columns. Column 1 is muted.
    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0, 1, 2 });
    context.mixerService->muteColumn(0, 1, true);

    // Verify Mixer state
    QVERIFY(context.mixerService->shouldTrackPlay(0));
    QVERIFY(context.mixerService->shouldColumnPlay(0, 0));
    QVERIFY(!context.mixerService->shouldColumnPlay(0, 1)); // Muted
    QVERIFY(context.mixerService->shouldColumnPlay(0, 2));

    const auto instrument = makeInstrument();

    // Create Note Events
    // Note on Column 1 (Muted)
    NoteData noteData1 { 0, 1 };
    noteData1.setAsNoteOn(60, 100);
    Event event1 { 0, noteData1 };
    event1.setInstrument(instrument);

    // Act: Process Event on Muted Column
    context.worker.test_handleEvent(event1);

    // Assert:
    // 1. playNote should NOT be called (column is muted)
    QCOMPARE(context.midiService->playNoteCallCount, 0);

    // 2. stopAllNotes should NOT be called
    QCOMPARE(context.midiService->stopAllNotesCallCount, 0);

    // 3. The note-off is not sent either: nothing was started on the muted column, so
    // stopping it would only be a message to a port the song is not playing to.
    NoteData noteData2 { 0, 1 };
    noteData2.setAsNoteOff(60);
    Event event2 { 0, noteData2 };
    event2.setInstrument(instrument);
    context.worker.test_handleEvent(event2);
    QCOMPARE(context.midiService->stopNoteCallCount, 0);
}

void PlayerWorkerTest::test_trackMuteBehavior_shouldStopAllNotes()
{
    auto context = createWorkerContext();

    // Configure Mixer: Track 0 is muted.
    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0, 1, 2 });
    context.mixerService->muteTrack(0, true);

    QVERIFY(context.mixerService->shouldTrackPlay(0) == false);

    const auto instrument = std::make_shared<Instrument>("TestPort");

    // Note on Track 0
    NoteData noteData1 { 0, 0 };
    noteData1.setAsNoteOn(60, 100);
    Event event1 { 0, noteData1 };
    event1.setInstrument(instrument);

    // Act
    context.worker.test_handleEvent(event1);

    // Assert
    QCOMPARE(context.midiService->playNoteCallCount, 0);
    // Should NOT call stopAllNotes anymore (new logic)
    QCOMPARE(context.midiService->stopAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_columnMute_shouldStopActiveNote()
{
    auto context = createWorkerContext();

    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0, 1 });

    const auto instrument = makeInstrument();
    const auto event = makeNoteOnEvent(instrument);

    PlayerWorker::EventList events { event };
    PlayerWorker::Timing timing { 120, 4, 6 };
    context.worker.initialize(events, timing);

    // Play note
    context.worker.test_handleEvent(*event);

    // Mute Column 0
    context.mixerService->muteColumn(0, 0, true);

    // Check mixer state
    context.worker.callCheckMixerState();

    // Assert
    QCOMPARE(context.midiService->stopNoteCallCount, 1);
    QCOMPARE(context.midiService->stopAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_playback_shouldSendMidiEvents()
{
    auto context = createWorkerContext();

    // Configure Mixer: Track 0 enabled
    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0 });

    const auto instrument = makeInstrument();

    // Create Events
    // Note On
    NoteData noteDataOn { 0, 0 };
    noteDataOn.setAsNoteOn(64, 90);
    Event eventOn { 0, noteDataOn };
    eventOn.setInstrument(instrument);

    // Note Off
    NoteData noteDataOff { 0, 0 };
    noteDataOff.setAsNoteOff(64);
    Event eventOff { 10, noteDataOff };
    eventOff.setInstrument(instrument);

    // Act
    context.worker.test_handleEvent(eventOn);
    QCOMPARE(context.midiService->playNoteCallCount, 1);
    context.worker.test_handleEvent(eventOff);
    QCOMPARE(context.midiService->stopNoteCallCount, 1);
}

namespace {

//! Counts what reaches it from the playback thread, which the plain mock's counters cannot do
//! safely while that thread is still running.
class CountingMidiService : public MockMidiService
{
public:
    void playNote(InstrumentW, MidiNoteDataCR) override
    {
        // Slow enough that a stop nearly always arrives while one is in flight, which is the case
        // the barrier is there for
        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
        sent++;
    }

    std::atomic<int> sent { 0 };
};

} // namespace

void PlayerWorkerTest::test_stop_shouldLeaveNothingToBeSentAfterItReturns()
{
    // What a stop clears -- automation above all -- must not be written again by a tick that was
    // still being sent when the stop came. A jump in the play order is a stop and a play back to
    // back, and a straggler there carried the old pattern's automation into the new one.
    const auto midiService = std::make_shared<CountingMidiService>();
    const auto mixerService = std::make_shared<MixerService>();
    PlayerWorker worker { midiService, mixerService, nullptr };
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0 });

    const auto instrument = makeInstrument();
    PlayerWorker::EventList events;
    // A note on every tick for far longer than the test runs, so there is always one being sent
    for (quint64 tick = 0; tick < 100000; tick++) {
        NoteData noteData { 0, 0 };
        noteData.setAsNoteOn(60, 100);
        const auto event = std::make_shared<Event>(tick, noteData);
        event->setInstrument(instrument);
        events.push_back(event);
    }
    worker.initialize(events, PlayerWorker::Timing { 300, 4, 6 });

    std::thread playback { [&worker] { worker.play(); } };
    QTRY_VERIFY_WITH_TIMEOUT(midiService->sent > 10, 5000);

    worker.stop();
    const int atStop = midiService->sent;
    std::this_thread::sleep_for(std::chrono::milliseconds { 100 });
    const int later = midiService->sent;
    playback.join();

    QCOMPARE(later, atStop);
    QVERIFY(!worker.isPlaying());
}

void PlayerWorkerTest::test_stop_songRunsToItsEnd_shouldKeepTheControllers()
{
    // A song that ran to its end leaves its automation where it took it, over the tails
    auto context = createWorkerContext();
    const auto instrument = makeInstrument();
    const auto event = makeNoteOnEvent(instrument);
    context.worker.initialize({ event }, PlayerWorker::Timing { 300, 4, 6 });

    context.worker.play();

    QVERIFY(!context.worker.isPlaying());
    QCOMPARE(context.midiService->releaseAllNotesCallCount, 1);
    QCOMPARE(context.midiService->stopAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_stop_askedFor_shouldResetTheControllers()
{
    auto context = createWorkerContext();
    const auto instrument = makeInstrument();
    const auto event = makeNoteOnEvent(instrument);
    context.worker.initialize({ event }, PlayerWorker::Timing { 300, 4, 6 });

    context.worker.stop();

    QCOMPARE(context.midiService->stopAllNotesCallCount, 1);
    QCOMPARE(context.midiService->releaseAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_noteOff_soloedElsewhere_shouldNotBeSent()
{
    // The case this was written for: one track soloed, and the rest addressing gear that is
    // not connected. Their note-ons were already held back; their note-offs went out anyway,
    // and each one reached the backend for a port that is not there.
    auto context = createWorkerContext();

    context.mixerService->setTrackIndices({ 0, 1 });
    context.mixerService->setColumnIndices(0, { 0 });
    context.mixerService->setColumnIndices(1, { 0 });
    context.mixerService->soloTrack(1, true);

    QVERIFY(!context.mixerService->shouldColumnPlay(0, 0));

    const auto instrument = makeInstrument("MissingPort");

    for (const uint8_t note : { 60, 62, 64 }) {
        NoteData on { 0, 0 };
        on.setAsNoteOn(note, 100);
        Event onEvent { 0, on };
        onEvent.setInstrument(instrument);
        context.worker.test_handleEvent(onEvent);

        NoteData off { 0, 0 };
        off.setAsNoteOff(note);
        Event offEvent { 1, off };
        offEvent.setInstrument(instrument);
        context.worker.test_handleEvent(offEvent);
    }

    QCOMPARE(context.midiService->playNoteCallCount, 0);
    QCOMPARE(context.midiService->stopNoteCallCount, 0);
}

void PlayerWorkerTest::test_noteOff_mutedWhileSounding_shouldBeSentOnce()
{
    // A note that was already sounding when its column was muted still has to be stopped, and
    // stopped exactly once: checkMixerState() takes it, and the note-off that follows must not
    // send a second one.
    auto context = createWorkerContext();

    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0 });

    const auto instrument = makeInstrument();

    NoteData on { 0, 0 };
    on.setAsNoteOn(60, 100);
    Event onEvent { 0, on };
    onEvent.setInstrument(instrument);
    context.worker.test_handleEvent(onEvent);
    QCOMPARE(context.midiService->playNoteCallCount, 1);

    context.mixerService->muteColumn(0, 0, true);
    context.worker.callCheckMixerState();
    QCOMPARE(context.midiService->stopNoteCallCount, 1);

    NoteData off { 0, 0 };
    off.setAsNoteOff(60);
    Event offEvent { 1, off };
    offEvent.setInstrument(instrument);
    context.worker.test_handleEvent(offEvent);
    QCOMPARE(context.midiService->stopNoteCallCount, 1);
}

void PlayerWorkerTest::test_lookahead_externalInstruments_shouldStayZero()
{
    // A song played out of a port keeps the timing it always had. Running ahead would only put the
    // hardware in front of everything else, since the port is written from this thread either way.
    auto context = createWorkerContext();

    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0 });

    const auto instrument = makeInstrument("HardwarePort");
    const auto event = makeNoteOnEvent(instrument);

    context.worker.initialize({ event }, PlayerWorker::Timing { 120, 4, 6 });

    QCOMPARE(context.worker.lookahead(), std::chrono::steady_clock::duration::zero());
}

void PlayerWorkerTest::test_lookahead_internalInstruments_shouldRunAhead()
{
    auto context = createWorkerContext();
    context.midiService->internal = true;
    context.midiService->lookahead = std::chrono::milliseconds { 31 };

    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0 });

    const auto instrument = makeInstrument("Noteahead Internal Device 1");
    const auto event = makeNoteOnEvent(instrument);

    // Only known once the song is running: the lookahead comes from what the backend is doing.
    context.worker.initialize({ event }, PlayerWorker::Timing { 120, 4, 6 });
    QCOMPARE(context.worker.lookahead(), std::chrono::steady_clock::duration::zero());

    context.worker.callResolveLookahead();
    QCOMPARE(std::chrono::duration_cast<std::chrono::milliseconds>(context.worker.lookahead()).count(), 31LL);
}

void PlayerWorkerTest::test_lookahead_engineCannotSay_shouldNotRunAhead()
{
    // A stream that is not rendering has no timeline to be early on, so the song plays the way it
    // always did rather than holding notes back for a frame that will never come.
    auto context = createWorkerContext();
    context.midiService->internal = true;
    context.midiService->lookahead = std::nullopt;

    context.mixerService->setTrackIndices({ 0 });
    context.mixerService->setColumnIndices(0, { 0 });

    const auto instrument = makeInstrument("Noteahead Internal Device 1");
    const auto event = makeNoteOnEvent(instrument);

    context.worker.initialize({ event }, PlayerWorker::Timing { 120, 4, 6 });
    context.worker.callResolveLookahead();

    QCOMPARE(context.worker.lookahead(), std::chrono::steady_clock::duration::zero());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PlayerWorkerTest)
