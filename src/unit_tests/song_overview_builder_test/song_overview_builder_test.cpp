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

#include "song_overview_builder_test.hpp"

#include "../../application/position.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/song_overview_builder.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../view/controllers/song_overview_controller.hpp"

#include <QTest>

#include <memory>

namespace noteahead {

namespace {

void setNoteOn(Song & song, size_t pattern, size_t track, size_t column, size_t line, uint8_t pitch, uint8_t velocity = 100)
{
    NoteData noteData { track, column };
    noteData.setAsNoteOn(pitch, velocity);
    song.setNoteDataAtPosition(noteData, { .pattern = pattern, .track = track, .column = column, .line = line });
}

void setNoteOff(Song & song, size_t pattern, size_t track, size_t column, size_t line)
{
    NoteData noteData { track, column };
    noteData.setAsNoteOff();
    song.setNoteDataAtPosition(noteData, { .pattern = pattern, .track = track, .column = column, .line = line });
}

const SongOverview::Pattern & patternOf(const SongOverview & overview, size_t pattern)
{
    return overview.patterns.at(pattern);
}

} // namespace

void SongOverviewBuilderTest::test_build_note_shouldLastUntilTheNextNote()
{
    Song song;
    setNoteOn(song, 0, 0, 0, 0, 60);
    setNoteOn(song, 0, 0, 0, 4, 64);

    const auto overview = SongOverview::build(song);
    const auto & notes = patternOf(overview, 0).notes;

    QCOMPARE(notes.size(), size_t { 2 });
    QCOMPARE(notes.at(0).pitch, uint8_t { 60 });
    QCOMPARE(notes.at(0).startLine, 0.0);
    QCOMPARE(notes.at(0).lengthLines, 4.0);
}

void SongOverviewBuilderTest::test_build_note_shouldEndAtANoteOff()
{
    Song song;
    setNoteOn(song, 0, 0, 0, 0, 60);
    setNoteOff(song, 0, 0, 0, 2);

    const auto overview = SongOverview::build(song);
    const auto & notes = patternOf(overview, 0).notes;

    QCOMPARE(notes.size(), size_t { 1 });
    QCOMPARE(notes.at(0).lengthLines, 2.0);
}

void SongOverviewBuilderTest::test_build_note_shouldLastUntilThePatternEnd()
{
    Song song;
    setNoteOn(song, 0, 0, 0, 10, 60);

    const auto overview = SongOverview::build(song);
    const auto & notes = patternOf(overview, 0).notes;

    QCOMPARE(notes.size(), size_t { 1 });
    QCOMPARE(notes.at(0).lengthLines, static_cast<double>(song.lineCount(0) - 10));
}

void SongOverviewBuilderTest::test_build_note_withDelay_shouldStartInsideItsLine()
{
    Song song;
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    noteData.setDelay(static_cast<uint8_t>(song.ticksPerLine() / 2));
    song.setNoteDataAtPosition(noteData, { .pattern = 0, .track = 0, .column = 0, .line = 3 });

    const auto overview = SongOverview::build(song);

    QCOMPARE(patternOf(overview, 0).notes.at(0).startLine, 3.5);
}

void SongOverviewBuilderTest::test_build_columnsAndTracks_shouldKeepTheirOwnNotes()
{
    Song song;
    song.addColumn(0);
    setNoteOn(song, 0, 0, 0, 0, 60);
    setNoteOn(song, 0, 0, 1, 2, 67);
    setNoteOn(song, 0, 2, 0, 1, 36);
    setNoteOn(song, 0, 0, 0, 8, 62);

    const auto overview = SongOverview::build(song);
    const auto & notes = patternOf(overview, 0).notes;

    QCOMPARE(notes.size(), size_t { 4 });
    // The note in column 1 does not end the one in column 0
    const auto first = std::ranges::find_if(notes, [](auto && note) { return note.pitch == 60; });
    QVERIFY(first != notes.end());
    QCOMPARE(first->lengthLines, 8.0);
    const auto second = std::ranges::find_if(notes, [](auto && note) { return note.pitch == 67; });
    QCOMPARE(second->column, size_t { 1 });
    const auto bass = std::ranges::find_if(notes, [](auto && note) { return note.pitch == 36; });
    QCOMPARE(bass->track, size_t { 2 });
    // Drawn track by track, so the later track's notes come last and are on top
    QCOMPARE(notes.back().track, size_t { 2 });
}

void SongOverviewBuilderTest::test_build_playOrder_shouldBuildARepeatedPatternOnce()
{
    Song song;
    song.createPattern(1);
    song.setLength(3);
    song.setPatternAtSongPosition(0, 0);
    song.setPatternAtSongPosition(1, 1);
    song.setPatternAtSongPosition(2, 0);

    const auto overview = SongOverview::build(song);

    QCOMPARE(overview.rows.size(), size_t { 3 });
    QCOMPARE(overview.rows.at(1).pattern, size_t { 1 });
    QCOMPARE(overview.rows.at(2).pattern, size_t { 0 });
    QCOMPARE(overview.rows.at(2).position, size_t { 2 });
    QCOMPARE(overview.patterns.size(), size_t { 2 });
}

void SongOverviewBuilderTest::test_build_patternsOfDifferentLengths_shouldFollowOnTheTimeline()
{
    Song song;
    song.createPattern(1);
    song.setLineCount(1, 32);
    song.setLength(2);
    song.setPatternAtSongPosition(1, 1);

    const auto overview = SongOverview::build(song);

    QCOMPARE(patternOf(overview, 1).lineCount, size_t { 32 });
    QCOMPARE(overview.rows.at(1).startLine, song.lineCount(0));
    QCOMPARE(overview.totalLines, song.lineCount(0) + 32);
    QCOMPARE(overview.rowAt(static_cast<double>(song.lineCount(0)) - 0.5), std::optional<size_t> { 0 });
    QCOMPARE(overview.rowAt(static_cast<double>(song.lineCount(0))), std::optional<size_t> { 1 });
    QVERIFY(!overview.rowAt(static_cast<double>(overview.totalLines)).has_value());
}

void SongOverviewBuilderTest::test_build_skippedPosition_shouldBeMarked()
{
    Song song;
    song.setLength(2);
    song.setSkipped(1, true);

    const auto overview = SongOverview::build(song);

    QVERIFY(!overview.rows.at(0).skipped);
    QVERIFY(overview.rows.at(1).skipped);
}

void SongOverviewBuilderTest::test_build_pitchRange_shouldSpanTheTracksNotes()
{
    Song song;
    song.createPattern(1);
    song.setLength(2);
    song.setPatternAtSongPosition(1, 1);
    setNoteOn(song, 0, 0, 0, 0, 40);
    setNoteOn(song, 1, 0, 0, 0, 90);
    setNoteOn(song, 0, 1, 0, 0, 36);

    const auto overview = SongOverview::build(song);

    QCOMPARE(overview.tracks.at(0).lowestPitch, uint8_t { 40 });
    QCOMPARE(overview.tracks.at(0).highestPitch, uint8_t { 90 });
    // A lane of one drum still gets an octave around it
    QCOMPARE(overview.tracks.at(1).highestPitch - overview.tracks.at(1).lowestPitch, 11);
    QVERIFY(overview.tracks.at(1).lowestPitch <= 36 && overview.tracks.at(1).highestPitch >= 36);
}

void SongOverviewBuilderTest::test_build_emptySong_shouldHaveADefaultRange()
{
    Song song;

    const auto overview = SongOverview::build(song);

    QCOMPARE(overview.rows.size(), size_t { 1 });
    QVERIFY(patternOf(overview, 0).notes.empty());
    QVERIFY(overview.tracks.at(0).highestPitch > overview.tracks.at(0).lowestPitch);
}

void SongOverviewBuilderTest::test_noteAt_overlappingNotes_shouldReturnTheTopOne()
{
    Song song;
    song.addColumn(0);
    setNoteOn(song, 0, 0, 0, 0, 60);
    setNoteOn(song, 0, 0, 1, 0, 60);
    setNoteOn(song, 0, 1, 0, 0, 60);

    const auto overview = SongOverview::build(song);
    const auto note = overview.noteAt(0, 0, 1.0, 60.0, 0.5);

    QVERIFY(note.has_value());
    QCOMPARE(note->track, size_t { 0 });
    QCOMPARE(note->column, size_t { 1 });
}

void SongOverviewBuilderTest::test_noteAt_outsideTolerance_shouldReturnNothing()
{
    Song song;
    setNoteOn(song, 0, 0, 0, 0, 60);
    setNoteOff(song, 0, 0, 0, 4);

    const auto overview = SongOverview::build(song);

    QVERIFY(overview.noteAt(0, 0, 1.0, 60.4, 0.5).has_value());
    QVERIFY(!overview.noteAt(0, 0, 1.0, 62.0, 0.5).has_value());
    QVERIFY(!overview.noteAt(0, 0, 4.0, 60.0, 0.5).has_value());
    // Another track's lane does not see it
    QVERIFY(!overview.noteAt(0, 1, 1.0, 60.0, 0.5).has_value());
}

void SongOverviewBuilderTest::test_controller_targetAt_note_shouldGoToItsTrackColumnAndStart()
{
    const auto editorService = std::make_shared<EditorService>();
    setNoteOn(*editorService->song(), 0, 3, 0, 8, 60);
    SongOverviewController controller { editorService, nullptr, nullptr };
    controller.setActive(true);

    const auto target = controller.targetAt(0, 3, 10.5, 60.0, 0.5);

    QVERIFY(target["valid"].toBool());
    QVERIFY(target["isNote"].toBool());
    QCOMPARE(target["track"].toULongLong(), quint64 { 3 });
    QCOMPARE(target["column"].toULongLong(), quint64 { 0 });
    QCOMPARE(target["line"].toULongLong(), quint64 { 8 });
    QCOMPARE(target["noteName"].toString(), QString { "C-5" });
}

void SongOverviewBuilderTest::test_controller_targetAt_emptyLane_shouldGoToTheLanesTrack()
{
    const auto editorService = std::make_shared<EditorService>();
    editorService->requestPosition(0, 5, 0, 0, 0);
    SongOverviewController controller { editorService, nullptr, nullptr };
    controller.setActive(true);

    const auto target = controller.targetAt(0, 2, 12.7, 60.0, 0.5);

    QVERIFY(target["valid"].toBool());
    QVERIFY(!target["isNote"].toBool());
    QCOMPARE(target["track"].toULongLong(), quint64 { 2 });
    QCOMPARE(target["column"].toULongLong(), quint64 { 0 });
    QCOMPARE(target["line"].toULongLong(), quint64 { 12 });
}

void SongOverviewBuilderTest::test_controller_targetAt_strip_shouldKeepTheCursorsTrack()
{
    const auto editorService = std::make_shared<EditorService>();
    editorService->requestPosition(0, 5, 0, 0, 0);
    SongOverviewController controller { editorService, nullptr, nullptr };
    controller.setActive(true);

    const auto target = controller.targetAt(0, -1, 0.0, 0.0, 0.0);

    QVERIFY(target["valid"].toBool());
    QCOMPARE(target["track"].toULongLong(), quint64 { 5 });
    QCOMPARE(target["line"].toULongLong(), quint64 { 0 });
}

void SongOverviewBuilderTest::test_controller_targetAt_repeatedPattern_shouldGoToTheClickedRow()
{
    const auto editorService = std::make_shared<EditorService>();
    const auto song = editorService->song();
    song->createPattern(1);
    song->setLength(3);
    song->setPatternAtSongPosition(1, 1);
    song->setPatternAtSongPosition(2, 0);
    SongOverviewController controller { editorService, nullptr, nullptr };
    controller.setActive(true);

    const auto target = controller.targetAt(2, 0, 0.0, 60.0, 0.5);

    QCOMPARE(target["position"].toULongLong(), quint64 { 2 });
    QCOMPARE(target["pattern"].toULongLong(), quint64 { 0 });
}

void SongOverviewBuilderTest::test_controller_playhead_whenStopped_shouldFollowTheEditor()
{
    const auto editorService = std::make_shared<EditorService>();
    const auto song = editorService->song();
    song->createPattern(1);
    song->setLength(2);
    song->setPatternAtSongPosition(1, 1);
    SongOverviewController controller { editorService, nullptr, nullptr };
    controller.setActive(true);

    editorService->setSongPosition(1);
    editorService->requestPosition(1, 0, 0, 6, 0);

    QCOMPARE(controller.playheadRow(), 1);
    QCOMPARE(controller.playheadLine(), 6.0);
    QCOMPARE(controller.playheadSongLine(), static_cast<double>(song->lineCount(0)) + 6.0);
}

void SongOverviewBuilderTest::test_controller_inactive_shouldNotBuild()
{
    const auto editorService = std::make_shared<EditorService>();
    SongOverviewController controller { editorService, nullptr, nullptr };

    QCOMPARE(controller.rowCount(), 0);
    controller.setActive(true);
    QCOMPARE(controller.rowCount(), 1);
    controller.setActive(false);
    QCOMPARE(controller.rowCount(), 0);
}

void SongOverviewBuilderTest::test_controller_notesStartingIn_shouldReturnTheNotesOfTheLines()
{
    const auto editorService = std::make_shared<EditorService>();
    const auto song = editorService->song();
    song->setLength(3);
    song->setSkipped(2, true);
    setNoteOn(*song, 0, 1, 0, 0, 60, 127);
    setNoteOn(*song, 0, 0, 0, 4, 48, 64);
    SongOverviewController controller { editorService, nullptr, nullptr };
    controller.setActive(true);

    // The second time the pattern plays, on the timeline after the first
    const auto atStart = controller.notesStartingIn(1, 0.0, 1.0);
    QCOMPARE(atStart.size(), 1);
    const auto note = atStart.at(0).toMap();
    QCOMPARE(note["lane"].toInt(), 1);
    QCOMPARE(note["pitch"].toInt(), 60);
    QCOMPARE(note["velocity"].toInt(), 127);
    QCOMPARE(note["songLine"].toDouble(), static_cast<double>(song->lineCount(0)));

    // A range of lines takes every note in it, and none past it
    QCOMPARE(controller.notesStartingIn(0, 0.0, 5.0).size(), 2);
    QCOMPARE(controller.notesStartingIn(0, 1.0, 4.0).size(), 0);
    // A skipped position plays nothing
    QCOMPARE(controller.notesStartingIn(2, 0.0, 5.0).size(), 0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SongOverviewBuilderTest)
