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

#include "song_test.hpp"

#include "../../application/position.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../domain/column_settings.hpp"
#include "../../domain/event.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/pattern.hpp"
#include "../../domain/song.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

void SongTest::test_autoNoteOffOffset_shouldCalculateCorrectOffset()
{
    Song song;
    song.setBeatsPerMinute(120);
    song.setLinesPerBeat(4);
    song.setAutoNoteOffOffset(125ms);

    QCOMPARE(song.autoNoteOffOffsetTicks(), 24);

    song.setAutoNoteOffOffset(250ms);

    QCOMPARE(song.autoNoteOffOffsetTicks(), 48);

    song.setBeatsPerMinute(240);

    QCOMPARE(song.autoNoteOffOffsetTicks(), 96);

    song.setLinesPerBeat(2);

    QCOMPARE(song.autoNoteOffOffsetTicks(), 48);

    song.setAutoNoteOffOffset(0ms);

    QCOMPARE(song.autoNoteOffOffsetTicks(), 0);
}

void SongTest::test_createPattern_columnAdded_shouldCreatePattern()
{
    Song song;

    song.addColumn(0);
    song.addColumn(2);
    song.addColumn(4);
    song.addColumn(6);
    song.addColumn(6);
    song.createPattern(1);

    QCOMPARE(song.patternCount(), 2);
    QCOMPARE(song.trackCount(0), song.trackCount(1));
    for (size_t i = 0; i < song.trackCount(0); i++) {
        QCOMPARE(song.columnCount(0, i), song.columnCount(1, i));
    }
    QCOMPARE(song.lineCount(0), song.lineCount(1));
}

void SongTest::test_hasData_emptySong_hasNoData()
{
    Song song;

    QVERIFY(!song.hasData());
}

void SongTest::test_hasData_noteOnAdded_shouldHaveData()
{
    Song song;

    song.noteDataAtPosition({ 0, 0, 0, 42, 0 })->setAsNoteOn(60, 100);

    QVERIFY(song.hasData());
}

void SongTest::test_hasData_noteOffAdded_shouldHaveData()
{
    Song song;

    song.noteDataAtPosition({ 0, 0, 0, 42, 0 })->setAsNoteOff(60);

    QVERIFY(song.hasData());
}

void SongTest::test_nextNoteDataOnSameColumn_noteOn_shouldFindNoteData()
{
    Song song;
    const Position notePosition = { 0, 1, 0, 8, 0 };
    song.noteDataAtPosition(notePosition)->setAsNoteOn(60, 100);

    const Position testPosition1 = { 0, 0, 0, 0, 0 };
    auto nextPosition = song.nextNoteDataOnSameColumn(testPosition1);

    QCOMPARE(nextPosition, testPosition1);

    const Position testPosition2 = { 0, 1, 0, 4, 0 };
    nextPosition = song.nextNoteDataOnSameColumn(testPosition2);

    QCOMPARE(nextPosition, notePosition);
}

void SongTest::test_nextNoteDataOnSameColumn_noteOff_shouldFindNoteData()
{
    Song song;
    const Position notePosition = { 0, 1, 0, 8, 0 };
    song.noteDataAtPosition(notePosition)->setAsNoteOff();

    const Position testPosition1 = { 0, 0, 0, 0, 0 };
    auto nextPosition = song.nextNoteDataOnSameColumn(testPosition1);

    QCOMPARE(nextPosition, testPosition1);

    const Position testPosition2 = { 0, 1, 0, 4, 0 };
    nextPosition = song.nextNoteDataOnSameColumn(testPosition2);

    QCOMPARE(nextPosition, notePosition);
}

void SongTest::test_prevNoteDataOnSameColumn_noteOn_shouldFindNoteData()
{
    Song song;
    const Position notePosition = { 0, 1, 0, 8, 0 };
    song.noteDataAtPosition(notePosition)->setAsNoteOn(60, 100);

    const Position testPosition1 = { 0, 0, 0, 0, 0 };
    auto prevPosition = song.prevNoteDataOnSameColumn(testPosition1);

    QCOMPARE(prevPosition, testPosition1);

    const Position testPosition2 = { 0, 1, 0, 12, 0 };
    prevPosition = song.prevNoteDataOnSameColumn(testPosition2);

    QCOMPARE(prevPosition, notePosition);
}

void SongTest::test_prevNoteDataOnSameColumn_noteOff_shouldFindNoteData()
{
    Song song;
    const Position notePosition = { 0, 1, 0, 8, 0 };
    song.noteDataAtPosition(notePosition)->setAsNoteOff();

    const Position testPosition1 = { 0, 0, 0, 0, 0 };
    auto prevPosition = song.prevNoteDataOnSameColumn(testPosition1);

    QCOMPARE(prevPosition, testPosition1);

    const Position testPosition2 = { 0, 1, 0, 12, 0 };
    prevPosition = song.prevNoteDataOnSameColumn(testPosition2);

    QCOMPARE(prevPosition, notePosition);
}

void SongTest::test_renderToEvents_clockEvents_shouldRenderClockEvents()
{
    Song song;
    auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 2);

    const auto instrument1 = std::make_shared<Instrument>("MyPort");
    song.setInstrument(0, instrument1);
    auto settings = instrument1->settings();
    settings.timing.sendMidiClock = true;
    instrument1->setSettings(settings);

    events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 194);

    const auto instrument2 = std::make_shared<Instrument>("MyPort");
    song.setInstrument(1, instrument2);
    settings = instrument2->settings();
    settings.timing.sendMidiClock = true;
    instrument2->setSettings(settings);

    events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 194);

    const auto instrument3 = std::make_shared<Instrument>("MyOtherPort");
    song.setInstrument(2, instrument3);
    settings = instrument3->settings();
    settings.timing.sendMidiClock = true;
    instrument3->setSettings(settings);

    events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 386);
}

void SongTest::test_renderToEvents_positiveDelaySet_shouldApplyDelay()
{
    Song song;
    song.setBeatsPerMinute(120);
    song.setLinesPerBeat(8);

    const auto instrument1 = std::make_shared<Instrument>("DelayedInstrument1");
    song.setInstrument(0, instrument1);
    auto settings1 = instrument1->settings();
    settings1.timing.delay = 42ms;
    instrument1->setSettings(settings1);

    const auto instrument2 = std::make_shared<Instrument>("DelayedInstrument2");
    song.setInstrument(1, instrument2);
    auto settings2 = instrument2->settings();
    settings2.timing.delay = 666ms;
    instrument2->setSettings(settings2);

    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(noteOnPosition)->setAsNoteOn(60, 100);

    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);
    const auto noteOn = events.at(1);
    const double msPerTick = 60000.0 / static_cast<double>(song.beatsPerMinute() * song.linesPerBeat() * song.ticksPerLine());
    const auto delay = static_cast<size_t>(std::round(static_cast<double>(instrument1->settings().timing.delay.count()) / msPerTick));
    QCOMPARE(noteOn->tick(), delay);
}

void SongTest::test_renderToEvents_negativeDelaySet_shouldApplyShiftedDelay()
{
    Song song;
    song.setBeatsPerMinute(120);
    song.setLinesPerBeat(8);

    const auto instrument1 = std::make_shared<Instrument>("DelayedInstrument1");
    song.setInstrument(0, instrument1);
    auto settings1 = instrument1->settings();
    settings1.timing.delay = 42ms;
    instrument1->setSettings(settings1);

    const auto instrument2 = std::make_shared<Instrument>("DelayedInstrument2");
    song.setInstrument(1, instrument2);
    auto settings2 = instrument2->settings();
    settings2.timing.delay = -666ms;
    instrument2->setSettings(settings2);

    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(noteOnPosition)->setAsNoteOn(60, 100);

    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);
    const auto noteOn = events.at(1);
    const double msPerTick = 60000.0 / static_cast<double>(song.beatsPerMinute() * song.linesPerBeat() * song.ticksPerLine());
    const auto delay = static_cast<size_t>(std::round(static_cast<double>(instrument1->settings().timing.delay.count() - instrument2->settings().timing.delay.count()) / msPerTick));
    QCOMPARE(noteOn->tick(), delay);
}

void SongTest::test_renderToEvents_noEvents_shouldAddStartAndEndOfSong()
{
    Song song;
    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 2);

    const auto startOfSong = events.at(0);
    QCOMPARE(startOfSong->tick(), 0);
    QCOMPARE(startOfSong->type(), Event::Type::StartOfSong);

    const auto endOfSong = events.at(1);
    QCOMPARE(endOfSong->tick(), song.lineCount(0) * song.ticksPerLine());
    QCOMPARE(endOfSong->type(), Event::Type::EndOfSong);
}

void SongTest::test_renderToEvents_noEvents_transportEnabled_shouldAddStartAndEndOfSong()
{
    Song song;
    const auto instrument1 = std::make_shared<Instrument>("");
    song.setInstrument(0, instrument1);
    auto settings = instrument1->settings();
    settings.timing.sendTransport = true;
    instrument1->setSettings(settings);

    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);

    auto event = events.at(0);
    QCOMPARE(event->tick(), 0);
    QCOMPARE(event->type(), Event::Type::StartOfSong);
    event = events.at(1);
    QCOMPARE(event->tick(), 0);
    QCOMPARE(event->type(), Event::Type::StartOfSong);
    event = events.at(2);
    QCOMPARE(event->tick(), song.lineCount(0) * song.ticksPerLine());
    QCOMPARE(event->type(), Event::Type::EndOfSong);
    event = events.at(3);
    QCOMPARE(event->tick(), song.lineCount(0) * song.ticksPerLine());
    QCOMPARE(event->type(), Event::Type::EndOfSong);
}

void SongTest::test_renderToEvents_noteOff_shouldMapNoteOff()
{
    Song song;
    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(noteOnPosition)->setAsNoteOn(60, 100);
    const Position noteOffPosition = { 0, 0, 0, 1, 0 };
    song.noteDataAtPosition(noteOffPosition)->setAsNoteOff();

    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);

    const auto noteOn = events.at(1);
    QCOMPARE(noteOn->tick(), 0 * song.ticksPerLine());
    QCOMPARE(noteOn->type(), Event::Type::NoteData);
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);
    QCOMPARE(noteOn->noteData()->velocity(), 100);

    const auto noteOff = events.at(2);
    QCOMPARE(noteOff->tick(), 1 * song.ticksPerLine());
    QCOMPARE(noteOff->type(), Event::Type::NoteData);
    QCOMPARE(noteOff->noteData()->type(), NoteData::Type::NoteOff);
    QCOMPARE(noteOff->noteData()->note(), 60);
    QCOMPARE(noteOff->noteData()->velocity(), 0);
}

void SongTest::test_renderToEvents_playOrderSet_shouldRenderMultiplePatterns()
{
    Song song;
    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(noteOnPosition)->setAsNoteOn(60, 100);
    const Position noteOffPosition = { 0, 0, 0, 1, 0 };
    song.noteDataAtPosition(noteOffPosition)->setAsNoteOff();

    song.setPatternAtSongPosition(0, 0);
    song.setPatternAtSongPosition(1, 0);

    song.setLength(1);
    auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);

    song.setLength(2);
    events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 6);

    auto noteOn = events.at(1);
    QCOMPARE(noteOn->tick(), 0 * song.ticksPerLine());
    QCOMPARE(noteOn->type(), Event::Type::NoteData);
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);
    QCOMPARE(noteOn->noteData()->velocity(), 100);

    auto noteOff = events.at(2);
    QCOMPARE(noteOff->tick(), 1 * song.ticksPerLine());
    QCOMPARE(noteOff->type(), Event::Type::NoteData);
    QCOMPARE(noteOff->noteData()->type(), NoteData::Type::NoteOff);
    QCOMPARE(noteOff->noteData()->note(), 60);
    QCOMPARE(noteOff->noteData()->velocity(), 0);

    noteOn = events.at(3);
    QCOMPARE(noteOn->tick(), song.ticksPerLine() * song.lineCount(0));
    QCOMPARE(noteOn->type(), Event::Type::NoteData);
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);
    QCOMPARE(noteOn->noteData()->velocity(), 100);

    noteOff = events.at(4);
    QCOMPARE(noteOff->tick(), song.ticksPerLine() * song.lineCount(0) + 1 * song.ticksPerLine());
    QCOMPARE(noteOff->type(), Event::Type::NoteData);
    QCOMPARE(noteOff->noteData()->type(), NoteData::Type::NoteOff);
    QCOMPARE(noteOff->noteData()->note(), 60);
    QCOMPARE(noteOff->noteData()->velocity(), 0);
}

void SongTest::test_renderToEvents_singleEvent_shouldRenderEvent()
{
    Song song;
    song.noteDataAtPosition({ 0, 0, 0, 42, 0 })->setAsNoteOn(60, 100);

    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);

    const auto startOfSong = events.at(0);
    QCOMPARE(startOfSong->tick(), 0);
    QCOMPARE(startOfSong->type(), Event::Type::StartOfSong);

    const auto noteOn = events.at(1);
    QCOMPARE(noteOn->tick(), 42 * song.ticksPerLine());
    QCOMPARE(noteOn->type(), Event::Type::NoteData);
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);
    QCOMPARE(noteOn->noteData()->velocity(), 100);

    const auto endOfSong = events.at(2);
    QCOMPARE(endOfSong->tick(), song.lineCount(0) * song.ticksPerLine());
    QCOMPARE(endOfSong->type(), Event::Type::EndOfSong);

    const auto danglingNoteOff = events.at(3);
    QCOMPARE(danglingNoteOff->tick(), song.lineCount(0) * song.ticksPerLine() + 1);
    QCOMPARE(danglingNoteOff->type(), Event::Type::NoteData);
    QCOMPARE(danglingNoteOff->noteData()->type(), NoteData::Type::NoteOff);
    QCOMPARE(danglingNoteOff->noteData()->note(), 60);
}

void SongTest::test_renderToEvents_sameColumn_shouldAddNoteOff()
{
    Song song;
    song.noteDataAtPosition({ 0, 0, 0, 21, 0 })->setAsNoteOn(60, 100);
    song.noteDataAtPosition({ 0, 0, 0, 42, 0 })->setAsNoteOn(60, 100);

    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 6);

    auto noteOn = events.at(1);
    QCOMPARE(noteOn->tick(), 21 * song.ticksPerLine());
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);

    const auto noteOff = events.at(2);
    QCOMPARE(noteOff->tick(), 42 * song.ticksPerLine() - song.autoNoteOffOffsetTicks());
    QCOMPARE(noteOff->noteData()->type(), NoteData::Type::NoteOff);
    QCOMPARE(noteOff->noteData()->note(), 60);

    noteOn = events.at(3);
    QCOMPARE(noteOn->tick(), 42 * song.ticksPerLine());
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);
}

void SongTest::test_renderToEvents_chordAutomation_shouldRenderChordNotes()
{
    Song song;
    auto settings = std::make_shared<ColumnSettings>();
    settings->chordAutomationSettings.note1.offset = 4;
    settings->chordAutomationSettings.note1.velocity = 80;
    settings->chordAutomationSettings.note2.offset = 7;
    settings->chordAutomationSettings.note2.velocity = 60;
    song.setColumnSettings(0, 0, settings);

    song.noteDataAtPosition({ 0, 0, 0, 0, 0 })->setAsNoteOn(60, 100);
    song.noteDataAtPosition({ 0, 0, 0, 1, 0 })->setAsNoteOff();

    const auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 8);

    const auto rootNoteOn = events.at(1);
    QCOMPARE(rootNoteOn->noteData()->note(), 60);
    QCOMPARE(rootNoteOn->noteData()->velocity(), 100);

    const auto chordNote1On = events.at(2);
    QCOMPARE(chordNote1On->noteData()->note(), 64);
    QCOMPARE(chordNote1On->noteData()->velocity(), 80);

    const auto chordNote2On = events.at(3);
    QCOMPARE(chordNote2On->noteData()->note(), 67);
    QCOMPARE(chordNote2On->noteData()->velocity(), 60);

    const auto rootNoteOff = events.at(4);
    QCOMPARE(rootNoteOff->noteData()->note(), 60);
    QCOMPARE(rootNoteOff->noteData()->type(), NoteData::Type::NoteOff);

    const auto chordNote1Off = events.at(5);
    QCOMPARE(chordNote1Off->noteData()->note(), 64);
    QCOMPARE(chordNote1Off->noteData()->type(), NoteData::Type::NoteOff);

    const auto chordNote2Off = events.at(6);
    QCOMPARE(chordNote2Off->noteData()->note(), 67);
    QCOMPARE(chordNote2Off->noteData()->type(), NoteData::Type::NoteOff);
}

void SongTest::test_renderToEvents_transposeSet_shouldApplyTranspose()
{
    Song song;

    const auto instrument1 = std::make_shared<Instrument>("TransposedInstrument1");
    song.setInstrument(0, instrument1);
    auto settings1 = instrument1->settings();
    settings1.transpose = 13;
    instrument1->setSettings(settings1);

    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(noteOnPosition)->setAsNoteOn(60, 100);

    auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);
    auto noteOn = events.at(1);
    QCOMPARE(noteOn->noteData()->note(), 73);

    settings1.transpose = -11;
    instrument1->setSettings(settings1);

    events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);
    noteOn = events.at(1);
    QCOMPARE(noteOn->noteData()->note(), 49);
}

void SongTest::test_renderToEvents_velocityJitterSet_shouldApplyVelocityJitter()
{
    Song song;

    const auto instrument = std::make_shared<Instrument>("");
    song.setInstrument(0, instrument);
    auto settings = instrument->settings();
    settings.midiEffects.velocityJitter = 50;
    instrument->setSettings(settings);

    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(noteOnPosition)->setAsNoteOn(60, 100);

    auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 4);
    auto noteOn = events.at(1);
    QCOMPARE(noteOn->noteData()->velocity(), 77);
}

void SongTest::test_renderToEvents_customNoteOffOffsetSet_shouldApplyCorrectOffset()
{
    Song song;

    const auto instrument = std::make_shared<Instrument>("");
    song.setBeatsPerMinute(120);
    song.setLinesPerBeat(4);
    song.setInstrument(0, instrument);
    song.setAutoNoteOffOffset(250ms); // Should not apply
    auto settings = instrument->settings();
    settings.timing.autoNoteOffOffset = 125ms;
    instrument->setSettings(settings);

    song.noteDataAtPosition({ 0, 0, 0, 0, 0 })->setAsNoteOn(60, 100);
    song.noteDataAtPosition({ 0, 0, 0, 8, 0 })->setAsNoteOn(60, 100);

    auto events = song.renderToEvents(std::make_shared<AutomationService>(), 0);
    QCOMPARE(events.size(), 6);

    auto noteOn = events.at(1);
    QCOMPARE(noteOn->tick(), 0);

    auto noteOff = events.at(2);
    noteOn = events.at(3);
    QCOMPARE(noteOn->tick() - noteOff->tick(), song.autoNoteOffOffsetTicks(settings.timing.autoNoteOffOffset.value()));
}

void SongTest::test_addTrack_shouldUseSmallestFreeId()
{
    Song song;
    // Initial state is 8 tracks (0-7)
    QCOMPARE(song.trackCount(0), 8);

    song.deleteTrack(1);
    song.deleteTrack(3);
    song.deleteTrack(5);

    QCOMPARE(song.trackCount(0), 5);

    song.addTrackToRightOf(0);
    QCOMPARE(song.trackCount(0), 6);
    auto indices = song.pattern(0)->trackIndices();
    QVERIFY(std::find(indices.begin(), indices.end(), 1) != indices.end());

    song.addTrackToRightOf(2);
    QCOMPARE(song.trackCount(0), 7);
    indices = song.pattern(0)->trackIndices();
    QVERIFY(std::find(indices.begin(), indices.end(), 3) != indices.end());

    song.addTrackToRightOf(4);
    QCOMPARE(song.trackCount(0), 8);
    indices = song.pattern(0)->trackIndices();
    QVERIFY(std::find(indices.begin(), indices.end(), 5) != indices.end());
}

void SongTest::test_trackByName_shouldReturnTrack()
{
    Song song;
    song.setTrackName(1, "Foo");

    QVERIFY(!song.trackByName("").has_value());
    QVERIFY(!song.trackByName("Bar").has_value());
    QVERIFY(song.trackByName("Foo").has_value());
    QCOMPARE(song.trackByName("Foo").value(), 1);
}

void SongTest::test_columnByName_shouldReturnColumn()
{
    Song song;
    song.setTrackName(1, "Foo");
    song.setColumnName(1, 0, "Bar");

    QVERIFY(!song.columnByName(0, "").has_value());
    QVERIFY(!song.columnByName(0, "Bar").has_value());
    QVERIFY(!song.columnByName(1, "Foo").has_value());
    QVERIFY(!song.columnByName(1, "").has_value());
    QVERIFY(song.columnByName(1, "Bar").has_value());
    QCOMPARE(song.columnByName(1, "Bar").value(), 0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SongTest)
