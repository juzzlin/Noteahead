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

#include "editor_service_undo_test.hpp"

#include "../../application/service/editor_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"

#include <QSignalSpy>

namespace noteahead {

void EditorServiceUndoTest::test_undoRedo_noteOn()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);

    // Initial state: No data
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());

    // Action: Note On
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
    QVERIFY(!editorService.canUndo());
    QVERIFY(editorService.canRedo());

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");
    QVERIFY(editorService.canUndo());
}

void EditorServiceUndoTest::test_undoRedo_noteOff()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);

    // Initial state: No data
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());

    // Action: Note Off
    editorService.requestNoteOffAtCurrentPosition();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "OFF");
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "OFF");
}

void EditorServiceUndoTest::test_undoRedo_velocityChange()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    
    // Move to velocity column
    editorService.requestPosition(0, 0, 0, 0, 3); // Last digit of velocity

    // Action: Change velocity digit
    // Velocity is 064. Cursor at last digit (4). Pressing '9' -> 069.
    editorService.requestDigitSetAtCurrentPosition(9);
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "069");
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "064");

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayVelocityAtPosition(0, 0, 0, 0), "069");
}

void EditorServiceUndoTest::test_undoRedo_deleteNote()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);

    // Action: Delete Note (non-shifting)
    editorService.requestNoteDeletionAtCurrentPosition(false);
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), "C-3");

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 0), editorService.noDataString());
}

void EditorServiceUndoTest::test_undoRedo_pasteColumn()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorService.requestColumnCopy();
    
    // Move to next column
    editorService.requestNewColumn(0);
    editorService.requestPosition(0, 0, 1, 0, 0);

    // Action: Paste Column
    editorService.requestColumnPaste();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C-3");
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), editorService.noDataString());

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 1, 0), "C-3");
}

void EditorServiceUndoTest::test_undoRedo_pasteTrack()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorService.requestTrackCopy();

    // Move to next track (create one)
    editorService.requestNewTrackToRight(); // Creates Track 1 (index 1)
    editorService.requestPosition(0, 1, 0, 0, 0);

    // Action: Paste Track
    editorService.requestTrackPaste();
    QCOMPARE(editorService.displayNoteAtPosition(0, 1, 0, 0), "C-3");
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 1, 0, 0), editorService.noDataString());

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 1, 0, 0), "C-3");
}

void EditorServiceUndoTest::test_undoRedo_pastePattern()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorService.requestPatternCopy();

    // Move to next pattern
    editorService.setCurrentPattern(1);
    editorService.requestPosition(1, 0, 0, 0, 0);

    // Action: Paste Pattern
    editorService.requestPatternPaste();
    QCOMPARE(editorService.displayNoteAtPosition(1, 0, 0, 0), "C-3");
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(1, 0, 0, 0), editorService.noDataString());

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayNoteAtPosition(1, 0, 0, 0), "C-3");
}

void EditorServiceUndoTest::test_undoRedo_pasteSelection()
{
    auto selectionService = std::make_shared<SelectionService>();
    EditorService editorService(selectionService);
    
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    
    selectionService->requestSelectionStart(0, 0, 0, 0);
    selectionService->requestSelectionEnd(0, 0, 0, 0);
    editorService.requestSelectionCopy();

    // Move
    editorService.requestPosition(0, 0, 0, 2, 0);

    // Action: Paste Selection
    editorService.requestSelectionPaste();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 2), "C-3");
    QVERIFY(editorService.canUndo());

    // Undo
    editorService.undo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 2), editorService.noDataString());

    // Redo
    editorService.redo();
    QCOMPARE(editorService.displayNoteAtPosition(0, 0, 0, 2), "C-3");
}

void EditorServiceUndoTest::test_undoRedo_canUndoRedoSignals()
{
    EditorService editorService;
    QSignalSpy canUndoSpy(&editorService, &EditorService::canUndoChanged);
    QSignalSpy canRedoSpy(&editorService, &EditorService::canRedoChanged);

    editorService.requestPosition(0, 0, 0, 0, 0);
    
    // Action -> canUndo: true
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    QCOMPARE(canUndoSpy.count(), 1);
    QVERIFY(editorService.canUndo());

    // Undo -> canUndo: false, canRedo: true
    editorService.undo();
    QCOMPARE(canUndoSpy.count(), 2);
    QCOMPARE(canRedoSpy.count(), 2);
    QVERIFY(!editorService.canUndo());
    QVERIFY(editorService.canRedo());

    // Redo -> canUndo: true, canRedo: false
    editorService.redo();
    QCOMPARE(canUndoSpy.count(), 3);
    QCOMPARE(canRedoSpy.count(), 3);
    QVERIFY(editorService.canUndo());
    QVERIFY(!editorService.canRedo());
}

void EditorServiceUndoTest::test_undoRedo_clearsOnNewSong()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    QVERIFY(editorService.canUndo());

    // Reset song (implicitly done via initialize or setSong)
    editorService.initialize();
    
    QVERIFY(!editorService.canUndo());
    QVERIFY(!editorService.canRedo());
}

void EditorServiceUndoTest::test_undoRedo_clearsOnPositionChange()
{
    EditorService editorService;
    // Ensure we have at least two tracks for testing track change
    editorService.requestNewTrackToRight();
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    QVERIFY(editorService.canUndo());

    // Track change within same pattern: should NOT clear
    editorService.requestPosition(0, 1, 0, 0, 0);
    QVERIFY(editorService.canUndo());

    // Pattern change: should clear
    editorService.setCurrentPattern(1);
    QVERIFY(!editorService.canUndo());
}

void EditorServiceUndoTest::test_undoRedo_clearsOnStructuralChange()
{
    EditorService editorService;
    editorService.requestPosition(0, 0, 0, 0, 0);
    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    QVERIFY(editorService.canUndo());

    // New column
    editorService.requestNewColumn(0);
    QVERIFY(!editorService.canUndo());

    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorService.requestNewTrackToRight();
    QVERIFY(!editorService.canUndo());

    editorService.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorService.setCurrentLineCount(32);
    QVERIFY(!editorService.canUndo());
}

} // namespace noteahead

QTEST_MAIN(noteahead::EditorServiceUndoTest)
