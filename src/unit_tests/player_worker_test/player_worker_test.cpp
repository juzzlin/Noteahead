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

#include "../../application/service/midi_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/player_worker.hpp"
#include "../../common/constants.hpp"
#include "../../domain/midi/midi_note_data.hpp"
#include "../../domain/tracker/event.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"

#include <QTest>
#include <memory>

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
class MockMidiService : public MidiService
{
public:
    MockMidiService()
      : MidiService(nullptr, nullptr, false)
    {
    }

    int stopAllNotesCallCount = 0;
    int playNoteCallCount = 0;
    int stopNoteCallCount = 0;

    void stopAllNotes(InstrumentW) override
    {
        stopAllNotesCallCount++;
    }

    void playNote(InstrumentW, MidiNoteDataCR) override
    {
        playNoteCallCount++;
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

void PlayerWorkerTest::test_mixerChange_shouldStopNotes()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    // Configure Mixer: Track 0 enabled
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0 });

    // Create Instrument
    const auto instrument { std::make_shared<Instrument>("TestPort") };
    instrument->setMidiAddress(MidiAddress { "TestPort", 0 });

    // Create Event on Track 0 to register instrument
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event { std::make_shared<Event>(0, noteData) };
    event->setInstrument(instrument);

    PlayerWorker::EventList events { event };
    PlayerWorker::Timing timing { 120, 4, 6 };

    worker.initialize(events, timing);

    // Simulate playback of the note to add it to m_activeNotes
    worker.test_handleEvent(*event);

    // Act: Mute Track 0
    mixerService->muteTrack(0, true);

    // Simulate mixer change handling
    worker.callCheckMixerState();

    // Assert: stopNote should be called for the active note on the muted track
    QCOMPARE(midiService->stopNoteCallCount, 1);
    QCOMPARE(midiService->stopAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_columnMuteBehavior_shouldNotStopAllNotes()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    // Configure Mixer: Track 0 has 3 columns. Column 1 is muted.
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0, 1, 2 });
    mixerService->muteColumn(0, 1, true);

    // Verify Mixer state
    QVERIFY(mixerService->shouldTrackPlay(0));
    QVERIFY(mixerService->shouldColumnPlay(0, 0));
    QVERIFY(!mixerService->shouldColumnPlay(0, 1)); // Muted
    QVERIFY(mixerService->shouldColumnPlay(0, 2));

    // Create Instrument
    auto instrument { std::make_shared<Instrument>("TestPort") };
    instrument->setMidiAddress(MidiAddress { "TestPort", 0 });

    // Create Note Events
    // Note on Column 1 (Muted)
    NoteData noteData1 { 0, 1 };
    noteData1.setAsNoteOn(60, 100);
    Event event1 { 0, noteData1 };
    event1.setInstrument(instrument);

    // Act: Process Event on Muted Column
    worker.test_handleEvent(event1);

    // Assert:
    // 1. playNote should NOT be called (column is muted)
    QCOMPARE(midiService->playNoteCallCount, 0);

    // 2. stopAllNotes should NOT be called
    QCOMPARE(midiService->stopAllNotesCallCount, 0);

    // 3. The note-off is not sent either: nothing was started on the muted column, so
    // stopping it would only be a message to a port the song is not playing to.
    NoteData noteData2 { 0, 1 };
    noteData2.setAsNoteOff(60);
    Event event2 { 0, noteData2 };
    event2.setInstrument(instrument);
    worker.test_handleEvent(event2);
    QCOMPARE(midiService->stopNoteCallCount, 0);
}

void PlayerWorkerTest::test_trackMuteBehavior_shouldStopAllNotes()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    // Configure Mixer: Track 0 is muted.
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0, 1, 2 });
    mixerService->muteTrack(0, true);

    QVERIFY(mixerService->shouldTrackPlay(0) == false);

    // Create Instrument
    const auto instrument { std::make_shared<Instrument>("TestPort") };

    // Note on Track 0
    NoteData noteData1 { 0, 0 };
    noteData1.setAsNoteOn(60, 100);
    Event event1 { 0, noteData1 };
    event1.setInstrument(instrument);

    // Act
    worker.test_handleEvent(event1);

    // Assert
    QCOMPARE(midiService->playNoteCallCount, 0);
    // Should NOT call stopAllNotes anymore (new logic)
    QCOMPARE(midiService->stopAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_columnMute_shouldStopActiveNote()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0, 1 });

    auto instrument { std::make_shared<Instrument>("TestPort") };
    instrument->setMidiAddress(MidiAddress { "TestPort", 0 });

    NoteData noteData { 0, 0 }; // Track 0, Col 0
    noteData.setAsNoteOn(60, 100);
    const auto event { std::make_shared<Event>(0, noteData) };
    event->setInstrument(instrument);

    PlayerWorker::EventList events { event };
    PlayerWorker::Timing timing { 120, 4, 6 };
    worker.initialize(events, timing);

    // Play note
    worker.test_handleEvent(*event);

    // Mute Column 0
    mixerService->muteColumn(0, 0, true);

    // Check mixer state
    worker.callCheckMixerState();

    // Assert
    QCOMPARE(midiService->stopNoteCallCount, 1);
    QCOMPARE(midiService->stopAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_playback_shouldSendMidiEvents()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    // Configure Mixer: Track 0 enabled
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0 });

    // Create Instrument
    const auto instrument { std::make_shared<Instrument>("TestPort") };
    instrument->setMidiAddress(MidiAddress { "TestPort", 0 });

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
    worker.test_handleEvent(eventOn);
    QCOMPARE(midiService->playNoteCallCount, 1);
    worker.test_handleEvent(eventOff);
    QCOMPARE(midiService->stopNoteCallCount, 1);
}

void PlayerWorkerTest::test_noteOff_soloedElsewhere_shouldNotBeSent()
{
    // The case this was written for: one track soloed, and the rest addressing gear that is
    // not connected. Their note-ons were already held back; their note-offs went out anyway,
    // and each one reached the backend for a port that is not there.
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    mixerService->setTrackIndices({ 0, 1 });
    mixerService->setColumnIndices(0, { 0 });
    mixerService->setColumnIndices(1, { 0 });
    mixerService->soloTrack(1, true);

    QVERIFY(!mixerService->shouldColumnPlay(0, 0));

    const auto instrument { std::make_shared<Instrument>("MissingPort") };
    instrument->setMidiAddress(MidiAddress { "MissingPort", 0 });

    for (const uint8_t note : { 60, 62, 64 }) {
        NoteData on { 0, 0 };
        on.setAsNoteOn(note, 100);
        Event onEvent { 0, on };
        onEvent.setInstrument(instrument);
        worker.test_handleEvent(onEvent);

        NoteData off { 0, 0 };
        off.setAsNoteOff(note);
        Event offEvent { 1, off };
        offEvent.setInstrument(instrument);
        worker.test_handleEvent(offEvent);
    }

    QCOMPARE(midiService->playNoteCallCount, 0);
    QCOMPARE(midiService->stopNoteCallCount, 0);
}

void PlayerWorkerTest::test_noteOff_mutedWhileSounding_shouldBeSentOnce()
{
    // A note that was already sounding when its column was muted still has to be stopped, and
    // stopped exactly once: checkMixerState() takes it, and the note-off that follows must not
    // send a second one.
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0 });

    const auto instrument { std::make_shared<Instrument>("TestPort") };
    instrument->setMidiAddress(MidiAddress { "TestPort", 0 });

    NoteData on { 0, 0 };
    on.setAsNoteOn(60, 100);
    Event onEvent { 0, on };
    onEvent.setInstrument(instrument);
    worker.test_handleEvent(onEvent);
    QCOMPARE(midiService->playNoteCallCount, 1);

    mixerService->muteColumn(0, 0, true);
    worker.callCheckMixerState();
    QCOMPARE(midiService->stopNoteCallCount, 1);

    NoteData off { 0, 0 };
    off.setAsNoteOff(60);
    Event offEvent { 1, off };
    offEvent.setInstrument(instrument);
    worker.test_handleEvent(offEvent);
    QCOMPARE(midiService->stopNoteCallCount, 1);
}

void PlayerWorkerTest::test_lookahead_externalInstruments_shouldStayZero()
{
    // A song played out of a port keeps the timing it always had. Running ahead would only put the
    // hardware in front of everything else, since the port is written from this thread either way.
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0 });

    const auto instrument { std::make_shared<Instrument>("HardwarePort") };
    instrument->setMidiAddress(MidiAddress { "HardwarePort", 0 });

    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event { std::make_shared<Event>(0, noteData) };
    event->setInstrument(instrument);

    worker.initialize({ event }, PlayerWorker::Timing { 120, 4, 6 });

    QCOMPARE(worker.lookahead(), std::chrono::steady_clock::duration::zero());
}

void PlayerWorkerTest::test_lookahead_internalInstruments_shouldRunAhead()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    midiService->internal = true;
    midiService->lookahead = std::chrono::milliseconds { 31 };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0 });

    const auto instrument { std::make_shared<Instrument>("Noteahead Internal Device 1") };
    instrument->setMidiAddress(MidiAddress { "Noteahead Internal Device 1", 0 });

    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event { std::make_shared<Event>(0, noteData) };
    event->setInstrument(instrument);

    // Only known once the song is running: the lookahead comes from what the backend is doing.
    worker.initialize({ event }, PlayerWorker::Timing { 120, 4, 6 });
    QCOMPARE(worker.lookahead(), std::chrono::steady_clock::duration::zero());

    worker.callResolveLookahead();
    QCOMPARE(std::chrono::duration_cast<std::chrono::milliseconds>(worker.lookahead()).count(), 31LL);
}

void PlayerWorkerTest::test_lookahead_engineCannotSay_shouldNotRunAhead()
{
    // A stream that is not rendering has no timeline to be early on, so the song plays the way it
    // always did rather than holding notes back for a frame that will never come.
    const auto midiService { std::make_shared<MockMidiService>() };
    midiService->internal = true;
    midiService->lookahead = std::nullopt;
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService, nullptr };

    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnIndices(0, { 0 });

    const auto instrument { std::make_shared<Instrument>("Noteahead Internal Device 1") };
    instrument->setMidiAddress(MidiAddress { "Noteahead Internal Device 1", 0 });

    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event { std::make_shared<Event>(0, noteData) };
    event->setInstrument(instrument);

    worker.initialize({ event }, PlayerWorker::Timing { 120, 4, 6 });
    worker.callResolveLookahead();

    QCOMPARE(worker.lookahead(), std::chrono::steady_clock::duration::zero());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PlayerWorkerTest)
