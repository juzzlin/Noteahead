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
#include "../../domain/event.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/midi_note_data.hpp"
#include "../../domain/note_data.hpp"

#include <memory>
#include <QTest>

namespace noteahead {

// Subclass to access protected members
class TestablePlayerWorker : public PlayerWorker
{
public:
    using PlayerWorker::PlayerWorker;

    void testHandleEvent(const Event & event)
    {
        handleEvent(event);
    }

    void callCheckMixerState()
    {
        checkMixerState();
    }
};

// Mock MidiService to capture calls
class MockMidiService : public MidiService
{
public:
    MockMidiService()
      : MidiService(nullptr, false)
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
};

void PlayerWorkerTest::test_mixerChange_shouldStopNotes()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService };

    // Configure Mixer: Track 0 enabled
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnCount(0, 1);

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
    worker.testHandleEvent(*event);

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
    TestablePlayerWorker worker { midiService, mixerService };

    // Configure Mixer: Track 0 has 3 columns. Column 1 is muted.
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnCount(0, 3);
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
    worker.testHandleEvent(event1);

    // Assert:
    // 1. playNote should NOT be called (column is muted)
    QCOMPARE(midiService->playNoteCallCount, 0);

    // 2. stopAllNotes should NOT be called
    QCOMPARE(midiService->stopAllNotesCallCount, 0);

    // Test Note Off (should always be processed)
    NoteData noteData2 { 0, 1 };
    noteData2.setAsNoteOff(60);
    Event event2 { 0, noteData2 };
    event2.setInstrument(instrument);
    worker.testHandleEvent(event2);
    QCOMPARE(midiService->stopNoteCallCount, 1);
}

void PlayerWorkerTest::test_trackMuteBehavior_shouldStopAllNotes()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService };

    // Configure Mixer: Track 0 is muted.
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnCount(0, 3);
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
    worker.testHandleEvent(event1);

    // Assert
    QCOMPARE(midiService->playNoteCallCount, 0);
    // Should NOT call stopAllNotes anymore (new logic)
    QCOMPARE(midiService->stopAllNotesCallCount, 0);
}

void PlayerWorkerTest::test_columnMute_shouldStopActiveNote()
{
    const auto midiService { std::make_shared<MockMidiService>() };
    const auto mixerService { std::make_shared<MixerService>() };
    TestablePlayerWorker worker { midiService, mixerService };

    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnCount(0, 2);

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
    worker.testHandleEvent(*event);

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
    TestablePlayerWorker worker { midiService, mixerService };

    // Configure Mixer: Track 0 enabled
    mixerService->setTrackIndices({ 0 });
    mixerService->setColumnCount(0, 1);

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
    worker.testHandleEvent(eventOn);
    QCOMPARE(midiService->playNoteCallCount, 1);
    worker.testHandleEvent(eventOff);
    QCOMPARE(midiService->stopNoteCallCount, 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PlayerWorkerTest)
