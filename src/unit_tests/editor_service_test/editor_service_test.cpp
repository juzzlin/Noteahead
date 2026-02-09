// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#include "editor_service_test.hpp"

#include <algorithm> // Required for std::sort
#include <vector>    // Required for std::vector

#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../infra/settings.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/column_settings.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/pattern.hpp"
#include "../../domain/side_chain_settings.hpp"
#include "../../domain/song.hpp"
#include "../../domain/track.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void EditorServiceTest::test_initialize_shouldInitializeCorrectly()
{
    EditorService editorService;

    // Track emitted signals
    QSignalSpy spyAboutToInitialize(&editorService, &EditorService::aboutToInitialize);
    QSignalSpy spyInitialized(&editorService, &EditorService::initialized);
    QSignalSpy spyStatusText(&editorService, &EditorService::statusTextRequested);
    QSignalSpy spySongChanged(&editorService, &EditorService::songChanged);
    QSignalSpy spyBpmChanged(&editorService, &EditorService::beatsPerMinuteChanged);
    QSignalSpy spyLpbChanged(&editorService, &EditorService::linesPerBeatChanged);
    QSignalSpy spyPatternChanged(&editorService, &EditorService::currentPatternChanged);
    QSignalSpy spyPatternAtCurrentPositionChanged(&editorService, &EditorService::patternAtCurrentSongPositionChanged);

    editorService.setSongPosition(1); // Alter song length and position

    QSignalSpy spySongLengthChanged(&editorService, &EditorService::songLengthChanged);
    QSignalSpy spySongPositionChanged(&editorService, &EditorService::songPositionChanged);

    QCOMPARE(spyPatternAtCurrentPositionChanged.count(), 1);

    editorService.initialize();

    // Assert: Check emitted signals
    QCOMPARE(spyAboutToInitialize.count(), 1);
    QCOMPARE(spyInitialized.count(), 1);
    QCOMPARE(spySongChanged.count(), 1);
    QCOMPARE(spyBpmChanged.count(), 1);
    QCOMPARE(spyLpbChanged.count(), 1);
    QCOMPARE(spyPatternChanged.count(), 1);
    QCOMPARE(spySongLengthChanged.count(), 1);
    QCOMPARE(spySongPositionChanged.count(), 1);
    QCOMPARE(spyPatternAtCurrentPositionChanged.count(), 2);

    // Verify status message
    QCOMPARE(spyStatusText.count(), 1);
    QCOMPARE(spyStatusText.takeFirst().at(0).toString(), QStringLiteral("An empty song initialized"));

    // Verify song initialization
    QVERIFY(editorService.song());
    QCOMPARE(editorService.songPosition(), 0);
}

void EditorServiceTest::test_defaultSong_shouldReturnCorrectProperties()
{
    EditorService editorService;

    const auto trackCount = 8;
    QCOMPARE(editorService.trackCount(), trackCount);
    for (size_t trackId = 0; trackId < trackCount; trackId++) {
        QCOMPARE(editorService.columnCount(trackId), 1);
        QCOMPARE(editorService.trackName(trackId), QString { "Track %1" }.arg(trackId + 1));
    }

    for (size_t trackId = 0; trackId < trackCount; trackId++) {
        QCOMPARE(editorService.columnCount(trackId), 1);
    }

    QCOMPARE(editorService.patternCount(), 1);
    QCOMPARE(editorService.lineCount(0), 64);

    QVERIFY(!editorService.canBeSaved());
    QVERIFY(!editorService.isModified());

    QCOMPARE(editorService.currentTime(), "00:00:00.000");
    QCOMPARE(editorService.duration(), "00:00:04.000");
}

void EditorServiceTest::test_defaultSong_shouldNotHaveNoteData()
{
    EditorService editorService;

    for (uint8_t pattern = 0; pattern < editorService.patternCount(); pattern++) {
        for (uint8_t track = 0; track < editorService.trackCount(); track++) {
            for (uint8_t column = 0; column < editorService.columnCount(track); column++) {
                for (uint8_t line = 0; line < editorService.lineCount(pattern); line++) {
                    QCOMPARE(editorService.displayNoteAtPosition(pattern, track, column, line), editorService.noDataString());
                    QCOMPARE(editorService.displayVelocityAtPosition(pattern, track, column, line), editorService.noDataString());
                }
            }
        }
    }
}

void EditorServiceTest::test_insertPattern_shouldInsertPattern()
{
    EditorService editorService;
    editorService.setPatternAtSongPosition(0, 1);
    editorService.insertPatternToPlayOrder();

    editorService.setSongPosition(0);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 0);
    editorService.setSongPosition(1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
}

void EditorServiceTest::test_removePattern_shouldRemovePattern()
{
    EditorService editorService;
    editorService.setPatternAtSongPosition(0, 1);
    editorService.insertPatternToPlayOrder();

    editorService.setSongPosition(0);
    editorService.removePatternFromPlayOrder();
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
}

void EditorServiceTest::test_columnCutPaste_equalSizes_shouldCopyColumn()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 129);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_columnCutPaste_shorterTarget_shouldCopyColumn()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 97);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_columnCopyPaste_equalSizes_shouldCopyColumn()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 65);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_columnCopyPaste_shorterTarget_shouldCopyColumn()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestColumnCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestColumnPaste();

    QCOMPARE(noteDataChangedSpy.count(), 33);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCutPaste_equalSizes_shouldCopyTrack()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 129);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCutPaste_shorterTarget_shouldCopyTrack()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 65);
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 97);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCopyPaste_equalSizes_shouldCopyTrack()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 65);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_trackCopyPaste_shorterTarget_shouldCopyTrack()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestTrackCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    const Position targetPosition = { 1, 2, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestTrackPaste();

    QCOMPARE(noteDataChangedSpy.count(), 33);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(targetPosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_patternCutPaste_equalSizes_shouldCopyPattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 513);
    editorService.setCurrentPattern(1);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 1025);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCutPaste_shorterTarget_shouldCopyPattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 513);
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 769);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCopyPaste_equalSizes_shouldCopyPattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 513);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCopyPaste_trackDeleted_shouldCopyPattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);
    QVERIFY(editorService.requestPosition({ 0, 1, 0, 0, 0 }));
    editorService.requestTrackDeletion();

    editorService.setIsModified(false);
    editorService.requestPatternCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 449);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_patternCopyPaste_shorterTarget_shouldCopyPattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 0, 0, 0, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    editorService.requestPatternCopy();
    QVERIFY(!editorService.isModified());
    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(32);
    editorService.requestPatternPaste();

    QCOMPARE(noteDataChangedSpy.count(), 257);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position targetPosition = { 1, 0, 0, 0, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(targetPosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(targetPosition), "064");
}

void EditorServiceTest::test_selectionCutPaste_shouldCopySelection()
{
    const auto selectionService = std::make_shared<SelectionService>();
    const auto settingsService = std::make_shared<SettingsService>();
    EditorService editorService { selectionService, settingsService };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 8, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    selectionService->requestSelectionStart(0, 1, 0, 4);
    selectionService->requestSelectionEnd(0, 1, 0, 12);
    editorService.requestSelectionCut();
    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 10);
    const Position targetPosition = { 0, 2, 0, 16, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestSelectionPaste();

    QCOMPARE(noteDataChangedSpy.count(), 19);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), editorService.noDataString());
    const Position pastedNotePosition = { 0, 2, 0, targetPosition.line + sourcePosition.line - selectionService->selectedPositions().at(0).line, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(pastedNotePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(pastedNotePosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_selectionCopyPaste_shouldCopySelection()
{
    const auto selectionService = std::make_shared<SelectionService>();
    const auto settingsService = std::make_shared<SettingsService>();
    EditorService editorService { selectionService, settingsService };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    const Position sourcePosition = { 0, 1, 0, 8, 0 };
    QVERIFY(editorService.requestPosition(sourcePosition));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 1);

    editorService.setIsModified(false);
    selectionService->requestSelectionStart(0, 1, 0, 4);
    selectionService->requestSelectionEnd(0, 1, 0, 12);
    editorService.requestSelectionCopy();
    QVERIFY(!editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 1);
    const Position targetPosition = { 0, 2, 0, 16, 0 };
    QVERIFY(editorService.requestPosition(targetPosition));
    editorService.requestSelectionPaste();

    QCOMPARE(noteDataChangedSpy.count(), 10);
    QCOMPARE(editorService.displayNoteAtPosition(sourcePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(sourcePosition), "064");
    const Position pastedNotePosition = { 0, 2, 0, targetPosition.line + sourcePosition.line - selectionService->selectedPositions().at(0).line, 0 };
    QCOMPARE(editorService.displayNoteAtPosition(pastedNotePosition), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(pastedNotePosition), "064");
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->column(), targetPosition.column);
    QCOMPARE(editorService.song()->noteDataAtPosition(pastedNotePosition)->track(), targetPosition.track);
}

void EditorServiceTest::test_requestCursorLeft_shouldWrapCorrectly()
{
    EditorService editorService;
    editorService.requestPosition({ 0, 0, 0, 0, 0 });
    editorService.requestCursorLeft();

    QCOMPARE(editorService.position().track, editorService.trackIndices().back());
}

void EditorServiceTest::test_requestDigitSetAtCurrentPosition_velocity_shouldChangeVelocity()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.isModified());

    editorService.requestCursorRight();
    QVERIFY(!editorService.requestDigitSetAtCurrentPosition(4)); // 464 not possible
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(1)); // 164 not possible => 127
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "127");
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(0)); // 027 possible
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "027");

    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(6));
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "067");
    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(8));
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "068");

    editorService.requestCursorLeft();
    editorService.requestCursorLeft();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(1)); // 168 not possible => 127
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "127");

    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(0));

    editorService.requestCursorRight();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(0));
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "100");

    editorService.requestCursorLeft();
    QVERIFY(editorService.requestDigitSetAtCurrentPosition(5)); // 150 not possible => 050
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "050");
}

void EditorServiceTest::test_velocity_input_hundreds_digit()
{
    EditorService editorService;
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));

    // Move to Hundreds digit of velocity (LineColumn 1)
    editorService.requestCursorRight();

    // Helper to set velocity and try input '1'
    auto checkInput = [&](uint8_t startVelocity, QString expectedDisplay) {
        // Reset to start velocity using note on
        editorService.requestCursorLeft(); // Back to note column
        editorService.requestNoteOnAtCurrentPosition(1, 3, startVelocity);
        editorService.requestCursorRight(); // Back to velocity hundreds

        QVERIFY(editorService.requestDigitSetAtCurrentPosition(1));
        QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), expectedDisplay);
    };

    checkInput(0, "100");
    checkInput(27, "127");
    checkInput(28, "127");
    checkInput(50, "127");
    checkInput(99, "127");
    checkInput(100, "100");
    checkInput(127, "127");
}

void EditorServiceTest::test_requestHorizontalScrollPositionChange_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy horizontalScrollChangeSpy(&editorService, &EditorService::horizontalScrollChanged);
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestHorizontalScrollBarPositionChange(0);
    QCOMPARE(editorService.horizontalScrollPosition(), 0);
    QVERIFY(editorService.isColumnVisible(0, 0));

    editorService.requestHorizontalScrollBarPositionChange(0.5);
    QCOMPARE(editorService.horizontalScrollPosition(), 2);
    QVERIFY(!editorService.isColumnVisible(0, 0));

    editorService.requestNewColumn(0);
    QCOMPARE(positionChangedSpy.count(), 2);
    editorService.requestHorizontalScrollBarPositionChange(0.5); // Not the exact threshold, but "enough"

    QCOMPARE(editorService.horizontalScrollPosition(), 3);
    QCOMPARE(horizontalScrollChangeSpy.count(), 2);
    QCOMPARE(positionChangedSpy.count(), 3);
}

void EditorServiceTest::test_requestNewColumn_shouldAddNewColumn()
{
    EditorService editorService;
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy columnAddedSpy { &editorService, &EditorService::columnAdded };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    editorService.requestNewColumn(0);

    QVERIFY(editorService.isModified());
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(columnAddedSpy.count(), 1);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 1);
    QCOMPARE(editorService.columnCount(0), 2);
}

void EditorServiceTest::test_requestColumnDeletion_shouldDeleteColumn()
{
    EditorService editorService;
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy columnDeletedSpy { &editorService, &EditorService::columnDeleted };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    editorService.requestNewColumn(0);
    QCOMPARE(editorService.columnCount(0), 2);
    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);

    editorService.requestColumnDeletion(0);

    QVERIFY(editorService.isModified());
    QCOMPARE(columnDeletedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 3);
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 2);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 2);
    QCOMPARE(editorService.columnCount(0), 1);
}

void EditorServiceTest::test_requestNewTrackToRight_shouldAddNewTrack()
{
    EditorService editorService;
    const auto initialTrackCount = editorService.trackCount();
    QVERIFY(editorService.requestPosition(0, 1, 0, 0, 0));
    QSignalSpy trackAddedSpy { &editorService, &EditorService::trackAdded };
    editorService.requestNewTrackToRight();

    const auto newIndex = initialTrackCount;
    QCOMPARE(editorService.trackCount(), initialTrackCount + 1);
    QCOMPARE(editorService.trackName(newIndex), "Track " + QString::number(newIndex + 1));
    QCOMPARE(editorService.trackPositionByIndex(newIndex), 2);
    QCOMPARE(editorService.trackIndexByPosition(2), newIndex);
    QCOMPARE(trackAddedSpy.count(), 1);
    QCOMPARE(trackAddedSpy.at(0).at(0).toUInt(), newIndex);

    editorService.requestNewColumn(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 2);

    QVERIFY(editorService.requestPosition(0, newIndex, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestColumnTranspose(1);
    editorService.requestTrackTranspose(1);
    const auto noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(0, newIndex, 1, 0);
    QCOMPARE(noteAndVelocity.at(0), "D-3");
    QCOMPARE(noteAndVelocity.at(1), "064");

    editorService.requestColumnDeletion(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 1);

    editorService.initialize();
    QCOMPARE(editorService.trackCount(), initialTrackCount);
}

void EditorServiceTest::test_requestNewTrackToLeft_shouldAddNewTrack()
{
    EditorService editorService;
    const auto initialTrackCount = editorService.trackCount();
    QVERIFY(editorService.requestPosition(0, 1, 0, 0, 0));
    QSignalSpy trackAddedSpy { &editorService, &EditorService::trackAdded };
    editorService.requestNewTrackToLeft();

    const auto newIndex = initialTrackCount;
    QCOMPARE(editorService.trackCount(), initialTrackCount + 1);
    QCOMPARE(editorService.trackName(newIndex), "Track " + QString::number(newIndex + 1));
    QCOMPARE(editorService.trackPositionByIndex(newIndex), 1);
    QCOMPARE(editorService.trackIndexByPosition(1), newIndex);
    QCOMPARE(trackAddedSpy.count(), 1);
    QCOMPARE(trackAddedSpy.at(0).at(0).toUInt(), newIndex);

    editorService.requestNewColumn(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 2);

    QVERIFY(editorService.requestPosition(0, newIndex, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestColumnTranspose(1);
    editorService.requestTrackTranspose(1);
    const auto noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(0, newIndex, 1, 0);
    QCOMPARE(noteAndVelocity.at(0), "D-3");
    QCOMPARE(noteAndVelocity.at(1), "064");

    editorService.requestColumnDeletion(newIndex);
    QCOMPARE(editorService.columnCount(newIndex), 1);

    editorService.initialize();
    QCOMPARE(editorService.trackCount(), initialTrackCount);
}

void EditorServiceTest::test_requestNewTrackToLeft_firstTrack_shouldAddNewTrack()
{
    EditorService editorService;
    const auto initialTrackCount = editorService.trackCount();
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QSignalSpy trackAddedSpy { &editorService, &EditorService::trackAdded };
    editorService.requestNewTrackToLeft();

    const auto newIndex = initialTrackCount;
    QCOMPARE(editorService.trackCount(), initialTrackCount + 1);
    QCOMPARE(editorService.trackName(newIndex), "Track " + QString::number(newIndex + 1));
    QCOMPARE(editorService.trackPositionByIndex(newIndex), 0);
    QCOMPARE(editorService.trackIndexByPosition(0), newIndex);
    QCOMPARE(trackAddedSpy.count(), 1);
    QCOMPARE(trackAddedSpy.at(0).at(0).toUInt(), newIndex);

    editorService.initialize();
    QCOMPARE(editorService.trackCount(), initialTrackCount);
}

void EditorServiceTest::test_requestTrackDeletion_firstTrack_shouldDeleteTrack()
{
    EditorService editorService;
    const auto initialTrackCount = editorService.trackCount();
    QSignalSpy trackDeletedSpy { &editorService, &EditorService::trackDeleted };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    editorService.requestTrackDeletion();

    QCOMPARE(editorService.trackCount(), initialTrackCount - 1);
    QVERIFY(editorService.isModified());
    QCOMPARE(trackDeletedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().track, editorService.trackIndices().at(0));
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(editorService.position().lineColumn, 0);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 1);
}

void EditorServiceTest::test_requestTrackDeletion_lastTrack_shouldDeleteTrack()
{
    EditorService editorService;
    const auto initialTrackCount = editorService.trackCount();
    QSignalSpy trackDeletedSpy { &editorService, &EditorService::trackDeleted };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy scrollBarSizeChangedSpy { &editorService, &EditorService::scrollBarHandleSizeChanged };
    QSignalSpy scrollBarStepSizeChangedSpy { &editorService, &EditorService::scrollBarStepSizeChanged };

    QVERIFY(editorService.requestPosition(0, editorService.trackCount() - 1, 0, 0, 0));
    editorService.requestTrackDeletion();

    QCOMPARE(editorService.trackCount(), initialTrackCount - 1);
    QVERIFY(editorService.isModified());
    QCOMPARE(trackDeletedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().track, editorService.trackIndices().at(initialTrackCount - 2));
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(editorService.position().lineColumn, 0);
    QCOMPARE(scrollBarSizeChangedSpy.count(), 1);
    QCOMPARE(scrollBarStepSizeChangedSpy.count(), 1);
}

void EditorServiceTest::test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.setIsModified(false);
    editorService.requestNoteDeletionAtCurrentPosition(false);

    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 2);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 4), editorService.noDataString());
}

void EditorServiceTest::test_requestNoteDeletionAtCurrentPosition_shouldDeleteNoteData_shouldShiftNotes()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.setIsModified(false);
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    editorService.requestNoteDeletionAtCurrentPosition(true);

    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 56);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.position().line, 9);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 9), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 9), "064");
}

void EditorServiceTest::test_requestNoteInsertionAtCurrentPosition_shouldInsertNoteData()
{
    EditorService editorService;
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 11, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(2, 3, 65));
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setIsModified(false);
    QVERIFY(editorService.requestPosition(0, 0, 0, 10, 0));
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    editorService.requestNoteInsertionAtCurrentPosition();

    QVERIFY(editorService.isModified());
    QCOMPARE(noteDataChangedSpy.count(), 54);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 10), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 10), editorService.noDataString());
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 11), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 11), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 12), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 12), "065");
    QCOMPARE(editorService.duration(), "00:00:04.000");
}

void EditorServiceTest::test_requestNoteOnAtCurrentPosition_shouldChangeNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };
    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.isModified());

    QCOMPARE(noteDataChangedSpy.count(), 1);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");

    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 88));
    QCOMPARE(noteDataChangedSpy.count(), 2);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "D-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "088");

    editorService.requestScroll(4);
    editorService.requestNoteOnAtCurrentPosition(2, 4, 72);
    QCOMPARE(noteDataChangedSpy.count(), 3);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 4), "C#4");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 4), "072");
}

void EditorServiceTest::test_requestNoteOnAtCurrentPosition_notOnNoteColumn_shouldNotChangeNoteData()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 1));
    QVERIFY(!editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QCOMPARE(noteDataChangedSpy.count(), 0);
    QVERIFY(!editorService.isModified());

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), editorService.noDataString());
}

void EditorServiceTest::test_requestColumnTranspose_shouldTransposeColumn()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestNewColumn(0);
    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 64));

    editorService.requestColumnTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 3);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "D#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
}

void EditorServiceTest::test_requestTrackTranspose_shouldTransposeTrack()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    editorService.requestNewColumn(0);
    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 64));

    editorService.requestTrackTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 4);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "D#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
}

void EditorServiceTest::test_requestPatternTranspose_shouldTransposePattern()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 1, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(3, 3, 64));

    editorService.requestPatternTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 4);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 1, 0, 0), "D#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 1, 0, 0), "064");
}

void EditorServiceTest::test_requestSelectionTranspose_shouldTransposeSelection()
{
    const auto selectionService = std::make_shared<SelectionService>();
    const auto settingsService = std::make_shared<SettingsService>();
    EditorService editorService { selectionService, settingsService };
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    editorService.requestNewColumn(0);

    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 4, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 8, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestSelectionTranspose(1); // No selection => should do nothing

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 4), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 4), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 8), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 8), "064");

    selectionService->requestSelectionStart(0, 0, 1, 0);
    selectionService->requestSelectionEnd(0, 0, 1, 4);

    editorService.requestSelectionTranspose(1);

    QCOMPARE(noteDataChangedSpy.count(), 5);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 4), "C#3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 4), "064");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 8), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 8), "064");
}

void EditorServiceTest::test_requestLinearVelocityInterpolationOnColumn_shouldInterpolateVelocities()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestLinearVelocityInterpolationOnColumn(0, 7, 0, 100, false);

    QCOMPARE(noteDataChangedSpy.count(), 6);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "000");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 3), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 3), "042");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 7), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 7), "100");
}

void EditorServiceTest::test_requestLinearVelocityInterpolationOnTrack_shouldInterpolateVelocities()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 0, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestNewColumn(0);

    QVERIFY(editorService.requestPosition(0, 0, 1, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));
    QVERIFY(editorService.requestPosition(0, 0, 1, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 64));

    editorService.requestLinearVelocityInterpolationOnTrack(0, 7, 0, 100, false);

    QCOMPARE(noteDataChangedSpy.count(), 12);

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "000");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 3), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 3), "042");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 7), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 7), "100");

    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 0), "000");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 3), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 3), "042");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 7), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 1, 7), "100");
}

void EditorServiceTest::test_requestLinearVelocityInterpolationOnColumn_shouldInterpolateVelocitiesAsPercentages()
{
    EditorService editorService;
    QSignalSpy noteDataChangedSpy { &editorService, &EditorService::noteDataAtPositionChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 100)); // Initial velocity 100
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 50));  // Initial velocity 50
    QVERIFY(editorService.requestPosition(0, 0, 0, 7, 0));
    QVERIFY(editorService.requestNoteOnAtCurrentPosition(1, 3, 20));  // Initial velocity 20

    // Interpolate from 50% to 150%
    editorService.requestLinearVelocityInterpolationOnColumn(0, 7, 50, 150, true);

    QCOMPARE(noteDataChangedSpy.count(), 6);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    // Line 0: 100 * 0.50 = 50
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "050");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 3), "C-3");
    // Line 3: Interpolated percentage at line 3: 50 + (150 - 50) * (3.0 / 7.0) = 50 + 100 * 0.42857 = 50 + 42.857 = 92.857
    //         Velocity at line 3: 50 * 92.857 / 100 = 46.4285 -> 46
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 3), "046");
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 7), "C-3");
    // Line 7: 20 * 1.50 = 30
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 7), "030");
}

void EditorServiceTest::test_requestPosition_invalidPosition_shouldNotChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };

    const auto neg = static_cast<size_t>(-1);
    QVERIFY(!editorService.requestPosition(neg, 0, 0, 0, 0));
    QVERIFY(!editorService.requestPosition(0, neg, 0, 0, 0));
    QVERIFY(!editorService.requestPosition(0, 0, neg, 0, 0));
    QVERIFY(!editorService.requestPosition(0, 0, 0, 0, neg));
    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(currentTimeChangedSpy.count(), 0);
}

void EditorServiceTest::test_requestPosition_validPosition_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    QVERIFY(editorService.requestPosition(0, 0, 0, 1, 0));
    QVERIFY(editorService.requestPosition(0, 0, 0, 2, 0));
    QVERIFY(editorService.requestPosition(0, 0, 0, 3, 0));
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(currentTimeChangedSpy.count(), 3);
}

void EditorServiceTest::test_resetSongPosition_firstTrackRemoved_shouldResetPosition()
{
    EditorService editorService;

    QVERIFY(editorService.requestPosition(0, 0, 0, 0, 0));
    editorService.requestTrackDeletion();
    editorService.resetSongPosition();

    QCOMPARE(editorService.position().pattern, 0);
    QCOMPARE(editorService.position().track, 1);
    QCOMPARE(editorService.position().column, 0);
    QCOMPARE(editorService.position().line, 0);
}

void EditorServiceTest::test_requestScroll_shouldChangePosition()
{
    EditorService editorService;

    editorService.setCurrentLineCount(65);
    editorService.requestPosition(0, 0, 0, 0, 0);

    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    QCOMPARE(editorService.position().line, 0);

    editorService.requestScroll(1);
    QCOMPARE(editorService.position().line, 1);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestScroll(0);
    QCOMPARE(editorService.position().line, 1);
    QCOMPARE(positionChangedSpy.count(), 2);

    editorService.requestScroll(-1);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(positionChangedSpy.count(), 3);

    editorService.requestScroll(-10);
    QCOMPARE(editorService.position().line, editorService.lineCount(editorService.currentPattern()) - 10);
    QCOMPARE(positionChangedSpy.count(), 4);

    editorService.requestScroll(10);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(positionChangedSpy.count(), 5);

    editorService.requestScroll(static_cast<int>(editorService.lineCount(editorService.currentPattern()) + 10));
    QCOMPARE(editorService.position().line, 10);
    QCOMPARE(positionChangedSpy.count(), 6);
}

void EditorServiceTest::test_requestScroll_shouldChangeCurrentTime()
{
    EditorService editorService;
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };
    QCOMPARE(currentTimeChangedSpy.count(), 0);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(editorService.currentTime(), "00:00:00.000");

    editorService.requestScroll(1);

    QCOMPARE(currentTimeChangedSpy.count(), 1);
    QCOMPARE(editorService.position().line, 1);
    QCOMPARE(editorService.currentTime(), "00:00:00.062");
}

void EditorServiceTest::test_requestPosition_shouldChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestPosition(0, 0, 0, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestPosition(0, 0, 1, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 1);

    editorService.requestNewColumn(0);
    QCOMPARE(positionChangedSpy.count(), 2);
    editorService.requestPosition(0, 0, 1, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 3);
    QCOMPARE(editorService.position().track, 0);
    QCOMPARE(editorService.position().column, 1);

    editorService.requestPosition(0, editorService.trackCount() - 1, 0, 0, 0);
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(editorService.position().track, editorService.trackCount() - 1);
}

void EditorServiceTest::test_requestPosition_shouldNotChangePosition()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    editorService.requestPosition(0, editorService.trackCount(), 0, 0, 0);

    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(editorService.position().track, 0);
}

void EditorServiceTest::test_setCurrentLineCount_shouldSetLineCount()
{
    EditorService editorService;
    QSignalSpy currentLineCountChangedSpy { &editorService, &EditorService::currentLineCountChanged };
    QSignalSpy durationChangedSpy { &editorService, &EditorService::durationChanged };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };

    // Min size
    editorService.setCurrentLineCount(0);
    QCOMPARE(currentLineCountChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(editorService.currentLineCount(), editorService.minLineCount());
    QVERIFY(editorService.isModified());
    QCOMPARE(durationChangedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:00.125");

    // No change
    editorService.setCurrentLineCount(editorService.minLineCount());
    QCOMPARE(currentLineCountChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(editorService.currentLineCount(), editorService.minLineCount());
    QCOMPARE(durationChangedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:00.125");

    // Expand
    editorService.setCurrentLineCount(256);
    QCOMPARE(currentLineCountChangedSpy.count(), 2);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(editorService.currentLineCount(), 256);
    QCOMPARE(durationChangedSpy.count(), 2);
    QCOMPARE(editorService.duration(), "00:00:16.000");

    // Shrink with cursor line over the bounds
    editorService.requestPosition(0, 0, 0, 200, 0);
    editorService.setCurrentLineCount(64);
    QCOMPARE(positionChangedSpy.count(), 5);
    QCOMPARE(currentLineCountChangedSpy.count(), 3);
    QCOMPARE(editorService.currentLineCount(), 64);
    QCOMPARE(editorService.position().line, 63);
    QCOMPARE(durationChangedSpy.count(), 3);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    // Max size
    editorService.setCurrentLineCount(editorService.maxLineCount());
    QCOMPARE(positionChangedSpy.count(), 6);
    QCOMPARE(currentLineCountChangedSpy.count(), 4);
    QCOMPARE(editorService.currentLineCount(), editorService.maxLineCount());
    QCOMPARE(durationChangedSpy.count(), 4);
    QCOMPARE(editorService.duration(), "00:01:02.437");

    // Beyond max size
    editorService.setCurrentLineCount(editorService.maxLineCount() + 1);
    QCOMPARE(positionChangedSpy.count(), 7);
    QCOMPARE(currentLineCountChangedSpy.count(), 5);
    QCOMPARE(editorService.currentLineCount(), editorService.maxLineCount());
    QCOMPARE(durationChangedSpy.count(), 4);
    QCOMPARE(editorService.duration(), "00:01:02.437");
}

void EditorServiceTest::test_setCurrentPattern_shouldCreatePattern()
{
    EditorService editorService;

    editorService.setCurrentLineCount(32); // New pattern should take the previous line count

    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy patternCreatedSpy { &editorService, &EditorService::patternCreated };

    editorService.setCurrentPattern(0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.patternCount(), 1);
    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(patternCreatedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:02.000");

    editorService.setCurrentPattern(1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternCount(), 2);
    QCOMPARE(editorService.lineCount(1), 32);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:02.000"); // Play order has not changed

    editorService.setCurrentPattern(0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.patternCount(), 2);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(patternCreatedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:02.000");

    editorService.setCurrentPattern(1);
    editorService.setCurrentLineCount(128); // New pattern should take the previous line count

    editorService.setCurrentPattern(2);

    QCOMPARE(editorService.currentPattern(), 2);
    QCOMPARE(editorService.patternCount(), 3);
    QCOMPARE(editorService.lineCount(1), 128);
    QCOMPARE(positionChangedSpy.count(), 5);
    QCOMPARE(patternCreatedSpy.count(), 2);
    QCOMPARE(editorService.duration(), "00:00:02.000");
}

void EditorServiceTest::test_setCurrentPattern_addColumnFirst_shouldCreatePattern()
{
    EditorService editorService;

    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy patternCreatedSpy { &editorService, &EditorService::patternCreated };

    editorService.setCurrentPattern(0);
    editorService.requestNewColumn(0);
    const auto newColumnCount = editorService.columnCount(0);
    editorService.setCurrentPattern(1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.columnCount(0), newColumnCount);
    QCOMPARE(editorService.patternCount(), 2);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(patternCreatedSpy.count(), 1);
}

void EditorServiceTest::test_setInstrumentSettings_shouldSetInstrumentSettings()
{
    EditorService editorService;
    QSignalSpy lineDataChangedSpy { &editorService, &EditorService::lineDataChanged };
    editorService.requestPosition(0, 1, 0, 0, 0);
    auto instrumentSettings = std::make_shared<InstrumentSettings>();
    instrumentSettings->patch = 42;

    editorService.setInstrumentSettingsAtCurrentPosition(instrumentSettings);

    QCOMPARE(lineDataChangedSpy.count(), 1);
    editorService.requestPosition(0, 1, 0, 0, 0);
    instrumentSettings = editorService.instrumentSettingsAtCurrentPosition();
    QVERIFY(instrumentSettings);
    QCOMPARE(instrumentSettings->patch, 42);
    QCOMPARE(instrumentSettings->track(), 1);
}

void EditorServiceTest::test_removeInstrumentSettings_shouldRemoveInstrumentSettings()
{
    EditorService editorService;
    QSignalSpy lineDataChangedSpy { &editorService, &EditorService::lineDataChanged };
    editorService.requestPosition(0, 1, 0, 0, 0);
    auto instrumentSettings = std::make_shared<InstrumentSettings>();
    instrumentSettings->patch = 42;
    editorService.setInstrumentSettingsAtCurrentPosition(instrumentSettings);
    QCOMPARE(lineDataChangedSpy.count(), 1);

    editorService.removeInstrumentSettingsAtCurrentPosition();
    QCOMPARE(lineDataChangedSpy.count(), 2);

    editorService.requestPosition(0, 1, 0, 0, 0);
    instrumentSettings = editorService.instrumentSettingsAtCurrentPosition();
    QVERIFY(!instrumentSettings);
}

void EditorServiceTest::test_setPatternAtSongPosition_shouldCreatePattern()
{
    EditorService editorService;

    QSignalSpy currentPatternChangedSpy { &editorService, &EditorService::currentPatternChanged };
    QSignalSpy durationChangedSpy { &editorService, &EditorService::durationChanged };
    QSignalSpy patternAtCurrentSongPositionChangedSpy { &editorService, &EditorService::patternAtCurrentSongPositionChanged };
    QSignalSpy patternCreatedChangedSpy { &editorService, &EditorService::patternCreated };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::songPositionChanged };

    editorService.setPatternAtSongPosition(0, 0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 0);
    QCOMPARE(editorService.patternCount(), 1);

    QCOMPARE(currentPatternChangedSpy.count(), 0);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 0);
    QCOMPARE(patternCreatedChangedSpy.count(), 0);
    QCOMPARE(positionChangedSpy.count(), 0);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setPatternAtSongPosition(0, 1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 0);
    QCOMPARE(editorService.duration(), "00:00:04.000");

    editorService.setPatternAtSongPosition(1, 1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(durationChangedSpy.count(), 1);
    QCOMPARE(editorService.duration(), "00:00:08.000");
}

void EditorServiceTest::test_setSongPosition_shouldChangePattern()
{
    EditorService editorService;

    QSignalSpy currentPatternChangedSpy { &editorService, &EditorService::currentPatternChanged };
    QSignalSpy patternAtCurrentSongPositionChangedSpy { &editorService, &EditorService::patternAtCurrentSongPositionChanged };
    QSignalSpy patternCreatedChangedSpy { &editorService, &EditorService::patternCreated };
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::songPositionChanged };

    editorService.setPatternAtSongPosition(1, 1);
    editorService.setSongPosition(1);

    QCOMPARE(editorService.currentPattern(), 1);
    QCOMPARE(editorService.position().pattern, 1);
    QCOMPARE(editorService.patternAtSongPosition(0), 0);
    QCOMPARE(editorService.patternAtSongPosition(1), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 1);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 1);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 1);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(songPositionChangedSpy.count(), 1);

    editorService.setSongPosition(0);

    QCOMPARE(editorService.currentPattern(), 0);
    QCOMPARE(editorService.position().pattern, 0);
    QCOMPARE(editorService.position().line, 0);
    QCOMPARE(editorService.patternAtSongPosition(0), 0);
    QCOMPARE(editorService.patternAtSongPosition(1), 1);
    QCOMPARE(editorService.patternAtCurrentSongPosition(), 0);
    QCOMPARE(editorService.patternCount(), 2);

    QCOMPARE(currentPatternChangedSpy.count(), 3);
    QCOMPARE(patternAtCurrentSongPositionChangedSpy.count(), 2);
    QCOMPARE(patternCreatedChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 4);
    QCOMPARE(songPositionChangedSpy.count(), 2);
}

void EditorServiceTest::test_setSongPosition_trackDeleted_shouldCreatePattern()
{
    EditorService editorService;
    editorService.setPatternAtSongPosition(0, 0);
    editorService.requestTrackDeletion();
    editorService.setPatternAtSongPosition(1, 1);
    QCOMPARE(editorService.patternCount(), 2);
}

void EditorServiceTest::test_setPatternName_shouldChangePatternName()
{
    EditorService editorService;

    editorService.setPatternName(0, "Foo");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.patternName(0), "Foo");
    QCOMPARE(editorService.currentPatternName(), "Foo");

    editorService.setCurrentPatternName("Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.patternName(0), "Bar");
    QCOMPARE(editorService.currentPatternName(), "Bar");
}

void EditorServiceTest::test_setColumnName_shouldChangeColumnName()
{
    EditorService editorService;

    editorService.setColumnName(0, 0, "Foo");
    editorService.setColumnName(1, 0, "Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.columnName(0, 0), "Foo");
    QCOMPARE(editorService.columnName(1, 0), "Bar");
}

void EditorServiceTest::test_setTrackName_shouldChangeTrackName()
{
    EditorService editorService;

    editorService.setTrackName(0, "Foo");
    editorService.setTrackName(1, "Bar");

    QVERIFY(editorService.isModified());
    QCOMPARE(editorService.trackName(0), "Foo");
    QCOMPARE(editorService.trackName(1), "Bar");
}

void EditorServiceTest::test_toXmlFromXml_playOrder()
{
    EditorService editorServiceOut;
    editorServiceOut.setPatternAtSongPosition(1, 11);
    editorServiceOut.setPatternAtSongPosition(2, 22);
    editorServiceOut.setPatternAtSongPosition(3, 33);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternAtSongPosition(1), 11);
    QCOMPARE(editorServiceIn.patternAtSongPosition(2), 22);
    QCOMPARE(editorServiceIn.patternAtSongPosition(3), 33);
}

void EditorServiceTest::test_toXmlFromXml_songProperties()
{
    EditorService editorServiceOut;
    editorServiceOut.setBeatsPerMinute(666);
    editorServiceOut.setLinesPerBeat(42);
    editorServiceOut.setPatternName(0, "patternName");
    editorServiceOut.setSongLength(16);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    QSignalSpy songChangedSpy { &editorServiceIn, &EditorService::songChanged };
    QSignalSpy positionChangedSpy { &editorServiceIn, &EditorService::positionChanged };
    QSignalSpy beatsPerMinuteChangedSpy { &editorServiceIn, &EditorService::beatsPerMinuteChanged };
    QSignalSpy linesPerBeatChangedSpy { &editorServiceIn, &EditorService::linesPerBeatChanged };
    editorServiceIn.fromXml(xml);

    QCOMPARE(songChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(beatsPerMinuteChangedSpy.count(), 1);
    QCOMPARE(linesPerBeatChangedSpy.count(), 1);
    QCOMPARE(editorServiceIn.beatsPerMinute(), editorServiceOut.beatsPerMinute());
    QCOMPARE(editorServiceIn.linesPerBeat(), editorServiceOut.linesPerBeat());
    QCOMPARE(editorServiceIn.patternName(0), editorServiceOut.patternName(0));
    QCOMPARE(editorServiceIn.songLength(), editorServiceOut.songLength());
}

void EditorServiceTest::test_toXmlFromXml_columnName_shouldLoadColumnName()
{
    EditorService editorServiceOut;
    editorServiceOut.setColumnName(0, 0, "columnName0_0");
    editorServiceOut.setColumnName(1, 0, "columnName1_0");

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.columnName(0, 0), editorServiceOut.columnName(0, 0));
    QCOMPARE(editorServiceIn.columnName(1, 0), editorServiceOut.columnName(1, 0));
}

void EditorServiceTest::test_toXmlFromXml_trackName_shouldLoadTrackName()
{
    EditorService editorServiceOut;
    editorServiceOut.setTrackName(0, "trackName0");
    editorServiceOut.setTrackName(1, "trackName1");

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackName(0), editorServiceOut.trackName(0));
    QCOMPARE(editorServiceIn.trackName(1), editorServiceOut.trackName(1));
}

void EditorServiceTest::test_toXmlFromXml_columnSettings_shouldSaveAndLoad()
{
    EditorService editorServiceOut;
    auto settingsOut = std::make_shared<ColumnSettings>();
    settingsOut->chordAutomationSettings.note1.offset = 4;
    settingsOut->chordAutomationSettings.note1.velocity = 80;
    settingsOut->chordAutomationSettings.note2.offset = 7;
    settingsOut->chordAutomationSettings.note2.velocity = 60;
    settingsOut->chordAutomationSettings.note3.offset = 12;
    settingsOut->chordAutomationSettings.note3.velocity = 90;
    editorServiceOut.setColumnSettings(1, 0, settingsOut);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    const auto settingsIn = editorServiceIn.columnSettings(1, 0);

    QVERIFY(settingsIn);
    QCOMPARE(settingsIn->chordAutomationSettings.note1.offset, settingsOut->chordAutomationSettings.note1.offset);
    QCOMPARE(settingsIn->chordAutomationSettings.note1.velocity, settingsOut->chordAutomationSettings.note1.velocity);
    QCOMPARE(settingsIn->chordAutomationSettings.note2.offset, settingsOut->chordAutomationSettings.note2.offset);
    QCOMPARE(settingsIn->chordAutomationSettings.note2.velocity, settingsOut->chordAutomationSettings.note2.velocity);
    QCOMPARE(settingsIn->chordAutomationSettings.note3.offset, settingsOut->chordAutomationSettings.note3.offset);
    QCOMPARE(settingsIn->chordAutomationSettings.note3.velocity, settingsOut->chordAutomationSettings.note3.velocity);
}

void EditorServiceTest::test_toXmlFromXml_automationService_midiCc_shouldLoadAutomationService()
{
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
    const auto comment = "MIDI CC Automation Test";

    AutomationService automationServiceOut;
    size_t id = 1;
    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                const auto newId = automationServiceOut.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment, track % 2 == 0, 8, 0);
                QCOMPARE(newId, id);
                id++;
            }
        }
    }

    AutomationService automationServiceIn;
    EditorService editorService;
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line0));
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line1));
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).size(), 1);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).controller(), controller);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line0, line0);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line1, line1);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value0, value0);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value1, value1);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).comment(), comment);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).enabled(), track % 2 == 0);
            }
        }
    }
}

void EditorServiceTest::test_toXmlFromXml_automationService_midiCc_withModulation_shouldLoadAutomationService()
{
    AutomationService automationServiceOut;
    const auto automationId = automationServiceOut.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {}, true, 8, 0);
    automationServiceOut.addMidiCcModulation(automationId, 1, 50.0f, 0.0f, true);

    AutomationService automationServiceIn;
    EditorService editorService;
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    const auto automation = automationServiceIn.midiCcAutomations().at(0);
    QCOMPARE(automation.modulation().cycles, 1.0f);
    QCOMPARE(automation.modulation().amplitude, 50.0f);
    QCOMPARE(automation.modulation().inverted, true);
}

void EditorServiceTest::test_toXmlFromXml_automationService_midiCc_noModulation_shouldLoadAutomationService()
{
    AutomationService automationServiceOut;
    automationServiceOut.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {}, true, 8, 0);

    AutomationService automationServiceIn;
    EditorService editorService;
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    const auto automation = automationServiceIn.midiCcAutomations().at(0);
    QCOMPARE(automation.modulation().cycles, 0.0f);
    QCOMPARE(automation.modulation().amplitude, 0.0f);
    QCOMPARE(automation.modulation().inverted, false);
}


void EditorServiceTest::test_toXmlFromXml_automationService_pitchBend_shouldLoadAutomationService()
{
    quint8 line0 = 4;
    quint8 line1 = 12;
    int value0 = -100;
    int value1 = +100;
    const auto comment = "Pitch Bend Automation Test";

    AutomationService automationServiceOut;
    size_t id = 1;
    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                const auto newId = automationServiceOut.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, comment, track % 2 == 0);
                QCOMPARE(newId, id);
                id++;
            }
        }
    }

    AutomationService automationServiceIn;
    EditorService editorService;
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line0));
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line1));
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).size(), 1);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line0, line0);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line1, line1);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value0, value0);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value1, value1);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).comment(), comment);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).enabled(), track % 2 == 0);
            }
        }
    }
}

void EditorServiceTest::test_toXmlFromXml_mixerService_shouldLoadMixerService()
{
    MixerService mixerServiceOut;
    mixerServiceOut.muteTrack(1, true);
    mixerServiceOut.soloTrack(2, true);
    mixerServiceOut.muteColumn(3, 0, true);
    mixerServiceOut.soloColumn(4, 1, true);
    mixerServiceOut.setColumnVelocityScale(1, 2, 42);
    mixerServiceOut.setTrackVelocityScale(3, 66);
    connect(&mixerServiceOut, &MixerService::trackIndicesRequested, this, [&]() {
        mixerServiceOut.setTrackIndices({ 0, 1, 2, 3, 4 });
    });
    connect(&mixerServiceOut, &MixerService::columnCountOfTrackRequested, this, [&](auto && trackIndex) {
        mixerServiceOut.setColumnCount(trackIndex, 3);
    });

    MixerService mixerServiceIn;
    EditorService editorService;
    connect(&editorService, &EditorService::mixerSerializationRequested, &mixerServiceOut, &MixerService::serializeToXml);
    connect(&editorService, &EditorService::mixerDeserializationRequested, &mixerServiceIn, &MixerService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    QVERIFY(mixerServiceIn.isTrackMuted(1));
    QVERIFY(mixerServiceIn.isTrackSoloed(2));
    QVERIFY(mixerServiceIn.isColumnMuted(3, 0));
    QVERIFY(mixerServiceIn.isColumnSoloed(4, 1));
    QCOMPARE(mixerServiceIn.columnVelocityScale(1, 2), 42);
    QCOMPARE(mixerServiceIn.trackVelocityScale(3), 66);
}

void EditorServiceTest::test_toXmlFromXml_instrumentSettings_shouldParseInstrumentSettings()
{
    EditorService editorServiceOut;
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);

    auto instrumentSettingsOut = std::make_shared<InstrumentSettings>();
    instrumentSettingsOut->patch = 42;
    instrumentSettingsOut->bank = { 10, 20, true };
    instrumentSettingsOut->transpose = -12;
    instrumentSettingsOut->timing.sendMidiClock = true;
    instrumentSettingsOut->timing.autoNoteOffOffset = std::chrono::milliseconds { 666 };
    instrumentSettingsOut->timing.delay = std::chrono::milliseconds { -666 };
    instrumentSettingsOut->midiEffects.velocityJitter = 42;
    instrumentSettingsOut->midiCcSettings = {
        { true, 7, 80 },
        { false, 10, 127 }
    };

    editorServiceOut.setInstrumentSettingsAtCurrentPosition(instrumentSettingsOut);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    editorServiceIn.requestPosition(0, 0, 0, 0, 0);
    const auto instrumentSettingsIn = editorServiceIn.instrumentSettingsAtCurrentPosition();

    QVERIFY(instrumentSettingsIn);

    QCOMPARE(instrumentSettingsIn->bank->byteOrderSwapped, instrumentSettingsOut->bank->byteOrderSwapped);
    QCOMPARE(instrumentSettingsIn->bank->lsb, instrumentSettingsOut->bank->lsb);
    QCOMPARE(instrumentSettingsIn->bank->msb, instrumentSettingsOut->bank->msb);
    QCOMPARE(instrumentSettingsIn->bank.has_value(), true);

    QCOMPARE(instrumentSettingsIn->patch, instrumentSettingsOut->patch);

    QCOMPARE(instrumentSettingsIn->transpose, instrumentSettingsOut->transpose);

    QCOMPARE(instrumentSettingsIn->timing.autoNoteOffOffset, instrumentSettingsOut->timing.autoNoteOffOffset);
    QCOMPARE(instrumentSettingsIn->timing.delay, instrumentSettingsOut->timing.delay);
    QCOMPARE(instrumentSettingsIn->timing.sendMidiClock, instrumentSettingsOut->timing.sendMidiClock);

    QCOMPARE(instrumentSettingsIn->midiEffects.velocityJitter, instrumentSettingsOut->midiEffects.velocityJitter);

    QCOMPARE(instrumentSettingsIn->midiCcSettings.size(), instrumentSettingsOut->midiCcSettings.size());
    for (size_t i = 0; i < instrumentSettingsOut->midiCcSettings.size(); ++i) {
        QCOMPARE(instrumentSettingsIn->midiCcSettings.at(i).enabled(), instrumentSettingsOut->midiCcSettings.at(i).enabled());
        QCOMPARE(instrumentSettingsIn->midiCcSettings.at(i).controller(), instrumentSettingsOut->midiCcSettings.at(i).controller());
        QCOMPARE(instrumentSettingsIn->midiCcSettings.at(i).value(), instrumentSettingsOut->midiCcSettings.at(i).value());
    }
}

void EditorServiceTest::test_toXmlFromXml_sideChainService_shouldLoadSideChainService()
{
    SideChainService sideChainServiceOut;
    SideChainSettings settings;
    settings.enabled = true;
    settings.sourceTrackIndex = 1;
    settings.sourceColumnIndex = 2;
    settings.lookahead = std::chrono::milliseconds(10);
    settings.release = std::chrono::milliseconds(100);
    settings.targets.push_back({ true, 7, 127, 0 });
    settings.targets.push_back({ false, 10, 100, 10 });
    sideChainServiceOut.setSettings(0, settings);

    SideChainService sideChainServiceIn;
    EditorService editorService;
    connect(&editorService, &EditorService::sideChainSerializationRequested, &sideChainServiceOut, &SideChainService::serializeToXml);
    connect(&editorService, &EditorService::sideChainDeserializationRequested, &sideChainServiceIn, &SideChainService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    const auto settingsIn = sideChainServiceIn.settings(0);

    QCOMPARE(settingsIn.enabled, settings.enabled);
    QCOMPARE(settingsIn.sourceTrackIndex, settings.sourceTrackIndex);
    QCOMPARE(settingsIn.sourceColumnIndex, settings.sourceColumnIndex);
    QCOMPARE(settingsIn.lookahead, settings.lookahead);
    QCOMPARE(settingsIn.release, settings.release);
    QCOMPARE(settingsIn.targets.size(), settings.targets.size());
    for (size_t i = 0; i < settings.targets.size(); ++i) {
        QCOMPARE(settingsIn.targets.at(i).enabled, settings.targets.at(i).enabled);
        QCOMPARE(settingsIn.targets.at(i).controller, settings.targets.at(i).controller);
        QCOMPARE(settingsIn.targets.at(i).targetValue, settings.targets.at(i).targetValue);
        QCOMPARE(settingsIn.targets.at(i).releaseValue, settings.targets.at(i).releaseValue);
    }
}

void EditorServiceTest::test_toXmlFromXml_noteData_noteOn()
{
    EditorService editorServiceOut;

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 1, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(2, 4, 100);

    editorServiceOut.requestPosition(0, 1, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 4, 127);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    auto noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 0), "064");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 2, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 2), "D-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 2), "080");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 1, 0, 0, 0 });
    QCOMPARE(noteData->track(), 1);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 1, 0, 0), "C#4");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 1, 0, 0), "100");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 1, 0, 2, 0 });
    QCOMPARE(noteData->track(), 1);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 1, 0, 2), "D-4");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 1, 0, 2), "127");
}

void EditorServiceTest::test_toXmlFromXml_noteData_delay_shouldSaveAndLoadDelay()
{
    EditorService editorServiceOut;
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorServiceOut.setDelayOnCurrentLine(12);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);
    QCOMPARE(editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 })->delay(), 12);

    editorServiceIn.requestPosition(0, 0, 0, 0, 0);
    QCOMPARE(editorServiceIn.delayAtCurrentPosition(), 12);

    editorServiceIn.requestPosition(0, 0, 0, 1, 0);
    QCOMPARE(editorServiceIn.delayAtCurrentPosition(), 0);
}

void EditorServiceTest::test_toXmlFromXml_noteData_noteOff()
{
    EditorService editorServiceOut;

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 0, 0, 4, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    auto noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 0), "OFF");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 0), "---");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 2, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 2), "D-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 2), "080");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 4, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 4), "OFF");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 4), "---");
}

void EditorServiceTest::test_toXmlFromXml_instrument_shouldParseInstrument()
{
    EditorService editorServiceOut;

    // Set up the instrument with all possible properties
    const auto instrumentOut = std::make_shared<Instrument>("Test Port");
    auto address = instrumentOut->midiAddress();
    address.setChannel(10); // Example channel
    instrumentOut->setMidiAddress(address);
    auto settings = instrumentOut->settings();
    settings.patch = 42; // Optional patch
    settings.bank = {
        static_cast<uint8_t>(21), // Bank LSB
        static_cast<uint8_t>(34), // Bank MSB
        true // Byte order swapped
    };
    instrumentOut->setSettings(settings);
    editorServiceOut.setInstrument(0, instrumentOut);

    // Serialize to XML
    const auto xml = editorServiceOut.toXml();

    // Deserialize from XML
    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    // Retrieve the instrument
    const auto instrumentIn = editorServiceIn.instrument(0);

    // Validate the instrument
    QVERIFY(instrumentIn);
    QCOMPARE(instrumentIn->midiAddress().portName(), instrumentOut->midiAddress().portName());
    QCOMPARE(instrumentIn->midiAddress().channel(), instrumentOut->midiAddress().channel());

    // Validate optional properties
    QCOMPARE(instrumentIn->settings().patch.has_value(), instrumentOut->settings().patch.has_value());
    if (instrumentIn->settings().patch && instrumentOut->settings().patch) {
        QCOMPARE(*instrumentIn->settings().patch, *instrumentOut->settings().patch);
    }

    QCOMPARE(instrumentIn->settings().bank.has_value(), instrumentOut->settings().bank.has_value());
    if (instrumentIn->settings().bank && instrumentOut->settings().bank) {
        QCOMPARE(instrumentIn->settings().bank->lsb, instrumentOut->settings().bank->lsb);
        QCOMPARE(instrumentIn->settings().bank->msb, instrumentOut->settings().bank->msb);
        QCOMPARE(instrumentIn->settings().bank->byteOrderSwapped, instrumentOut->settings().bank->byteOrderSwapped);
    }
}

void EditorServiceTest::test_toXmlFromXml_addTrack_shouldLoadSong()
{
    EditorService editorServiceOut;
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNewTrackToRight();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackCount(), editorServiceOut.trackCount());
    QCOMPARE(editorServiceIn.trackIndices(), editorServiceOut.trackIndices());
}

void EditorServiceTest::test_toXmlFromXml_removeTrack_shouldLoadSong()
{
    EditorService editorServiceOut;
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestTrackDeletion();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackCount(), editorServiceOut.trackCount());
    QCOMPARE(editorServiceIn.trackIndices(), editorServiceOut.trackIndices());
}

void EditorServiceTest::test_toXmlFromXml_template_shouldLoadTemplate()
{
    EditorService editorServiceOut;
    editorServiceOut.setPatternAtSongPosition(0, 15);

    const auto xml = editorServiceOut.toXmlAsTemplate();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);
}

void EditorServiceTest::test_toXmlFromXml_differentSongs_shouldLoadSongs()
{
    EditorService editorServiceOut;
    auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn;
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);

    editorServiceOut.setPatternAtSongPosition(0, 15);
    xml = editorServiceOut.toXml();

    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 15);
    QCOMPARE(editorServiceIn.patternCount(), 2);

    EditorService editorServiceOut2;
    xml = editorServiceOut2.toXml();

    editorServiceIn.fromXml(xml);
    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);
}

void EditorServiceTest::test_velocityAtPosition_shouldReturnCorrectVelocity()
{
    EditorService editorService;

    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 1, 50);
    QCOMPARE(editorService.velocityAtPosition(0, 0, 0, 0), 50);

    editorService.requestNoteOffAtCurrentPosition();
    QCOMPARE(editorService.velocityAtPosition(0, 0, 0, 0), 0);

    QCOMPARE(editorService.velocityAtPosition(0, 1, 0, 0), 0);
}

void EditorServiceTest::test_requestPositionByTick_shouldRespectUiUpdatesDisabledSetting()
{
    EditorService editorService;
    QSignalSpy positionChangedSpy { &editorService, &EditorService::positionChanged };
    QSignalSpy currentTimeChangedSpy { &editorService, &EditorService::currentTimeChanged };
    QSignalSpy songPositionChangedSpy { &editorService, &EditorService::songPositionChanged };

    // Enable disabling UI updates
    editorService.settingsService()->setUiUpdatesDisabledDuringPlayback(true);

    // Initial position
    QCOMPARE(editorService.songPosition(), 0);

    // Populate song position map manually as we don't have a PlayerService rendering
    editorService.song()->updateTickToSongPositionMapping(0, 0, 0, editorService.lineCount(0));

    // Tick update that triggers a line change
    const auto tick = editorService.ticksPerLine(); // Move to next line
    editorService.requestPositionByTick(tick);

    // Assert: Position should NOT change (UI update suppressed)
    QCOMPARE(positionChangedSpy.count(), 0);
    // Assert: Time should change
    QCOMPARE(currentTimeChangedSpy.count(), 1);
    // Assert: Song position should NOT change (we are still in the same pattern)
    QCOMPARE(songPositionChangedSpy.count(), 0);
    QCOMPARE(editorService.songPosition(), 0);

    // Reset setting
    editorService.settingsService()->setUiUpdatesDisabledDuringPlayback(false);

    // Another tick
    const auto nextTick = tick * 2;
    editorService.requestPositionByTick(nextTick);

    // Assert: Position should change now
    QCOMPARE(positionChangedSpy.count(), 1);
    QCOMPARE(currentTimeChangedSpy.count(), 2);
    // Assert: Song position still 0
    QCOMPARE(songPositionChangedSpy.count(), 0);
}

void EditorServiceTest::test_midiNotesAtPosition_shouldReturnCorrectNotes()
{
    auto selectionService = std::make_shared<SelectionService>();
    auto settingsService = std::make_shared<SettingsService>();
    auto mixerService = std::make_shared<MixerService>();
    EditorService editorService(selectionService, settingsService);
    editorService.setMixerService(mixerService);

    // Create a pattern with notes on different tracks/columns
    editorService.initialize();

    // Track 0, Column 0: Note 60
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 5, 100); // C5 -> 60

    // Track 0, Column 1 (add column first)
    editorService.requestNewColumn(0);
    editorService.requestPosition(0, 0, 1, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(3, 5, 100); // D5 -> 62

    // Track 1, Column 0: Note 64
    editorService.requestPosition(0, 1, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(5, 5, 100); // E5 -> 64

    // 1. Verify all notes are returned initially
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QCOMPARE(notesTrack0.size(), 2);
        // Ordering depends on implementation, but likely 60, 62
        bool found60 = std::find(notesTrack0.begin(), notesTrack0.end(), 60) != notesTrack0.end();
        bool found62 = std::find(notesTrack0.begin(), notesTrack0.end(), 62) != notesTrack0.end();
        QVERIFY(found60);
        QVERIFY(found62);

        const auto notesTrack1 = editorService.midiNotesAtPosition(0, 1, 0);
        QCOMPARE(notesTrack1.size(), 1);
        QCOMPARE(notesTrack1[0], 64);
    }

    // 2. Mute Track 0 -> Should return empty list for Track 0
    mixerService->muteTrack(0, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QVERIFY(notesTrack0.empty());
    }
    mixerService->muteTrack(0, false);

    // 3. Mute Track 0, Column 0 -> Should return only Note 62 (from Column 1)
    mixerService->muteColumn(0, 0, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QCOMPARE(notesTrack0.size(), 1);
        QCOMPARE(notesTrack0[0], 62);
    }
    mixerService->muteColumn(0, 0, false);

    // 4. Solo Track 0, Column 0 -> Should return only Note 60
    mixerService->soloColumn(0, 0, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QCOMPARE(notesTrack0.size(), 1);
        QCOMPARE(notesTrack0[0], 60);
    }
    mixerService->soloColumn(0, 0, false);

    // 5. Solo Track 1 -> Should return notes for Track 1, but Track 0 might still return notes if logic only checks "shouldTrackPlay" for THAT track.
    // wait, mixerService->shouldTrackPlay(0) should be FALSE if Track 1 is soloed.
    mixerService->soloTrack(1, true);
    {
        const auto notesTrack0 = editorService.midiNotesAtPosition(0, 0, 0);
        QVERIFY(notesTrack0.empty());

        const auto notesTrack1 = editorService.midiNotesAtPosition(0, 1, 0);
        QCOMPARE(notesTrack1.size(), 1);
        QCOMPARE(notesTrack1[0], 64);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::EditorServiceTest)
