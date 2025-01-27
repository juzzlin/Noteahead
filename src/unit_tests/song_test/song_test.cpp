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
#include "../../domain/event.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

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
    for (uint32_t i = 0; i < song.trackCount(0); i++) {
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

void SongTest::test_renderToEvents_noEvents_shouldAddStartAndEndOfSong()
{
    Song song;
    const auto events = song.renderToEvents();
    QCOMPARE(events.size(), 2);

    const auto startOfSong = events.at(0);
    QCOMPARE(startOfSong->tick(), 0);
    QCOMPARE(startOfSong->type(), Event::Type::StartOfSong);

    const auto endOfSong = events.at(1);
    QCOMPARE(endOfSong->tick(), song.lineCount(0) * song.ticksPerLine());
    QCOMPARE(endOfSong->type(), Event::Type::EndOfSong);
}

void SongTest::test_renderToEvents_noteOff_shouldMapNoteOff()
{
    Song song;
    const Position noteOnPosition = { 0, 0, 0, 0, 0 };
    song.noteDataAtPosition(noteOnPosition)->setAsNoteOn(60, 100);
    const Position noteOffPosition = { 0, 0, 0, 1, 0 };
    song.noteDataAtPosition(noteOffPosition)->setAsNoteOff();

    const auto events = song.renderToEvents();
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

    const auto events = song.renderToEvents();
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

    const auto events = song.renderToEvents();
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

    const auto events = song.renderToEvents();
    QCOMPARE(events.size(), 6);

    auto noteOn = events.at(1);
    QCOMPARE(noteOn->tick(), 21 * song.ticksPerLine());
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);

    const auto noteOff = events.at(2);
    QCOMPARE(noteOff->tick(), 42 * song.ticksPerLine() - song.autoNoteOffTickOffset());
    QCOMPARE(noteOff->noteData()->type(), NoteData::Type::NoteOff);
    QCOMPARE(noteOff->noteData()->note(), 60);

    noteOn = events.at(3);
    QCOMPARE(noteOn->tick(), 42 * song.ticksPerLine());
    QCOMPARE(noteOn->noteData()->type(), NoteData::Type::NoteOn);
    QCOMPARE(noteOn->noteData()->note(), 60);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SongTest)
