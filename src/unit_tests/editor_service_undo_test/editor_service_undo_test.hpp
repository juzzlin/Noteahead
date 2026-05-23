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

#ifndef EDITOR_SERVICE_UNDO_TEST_HPP
#define EDITOR_SERVICE_UNDO_TEST_HPP

#include <QObject>

namespace noteahead {

class EditorServiceUndoTest : public QObject
{
    Q_OBJECT

private slots:
    void test_undoRedo_noteOn_shouldUndoAndRedo();
    void test_undoRedo_noteOff_shouldUndoAndRedo();
    void test_undoRedo_velocityChange_shouldUndoAndRedo();
    void test_undoRedo_deleteNote_shouldUndoAndRedo();
    void test_undoRedo_insertNote_shouldUndoAndRedo();
    void test_undoRedo_backspaceNote_shouldUndoAndRedo();
    void test_undoRedo_noteOffWithRedundantRemoval_shouldUndoAndRedo();
    void test_undoRedo_pasteColumn_shouldUndoAndRedo();
    void test_undoRedo_pasteTrack_shouldUndoAndRedo();
    void test_undoRedo_pastePattern_shouldUndoAndRedo();
    void test_undoRedo_pasteSelection_shouldUndoAndRedo();
    void test_undoRedo_cutColumn_shouldUndoAndRedo();
    void test_undoRedo_cutTrack_shouldUndoAndRedo();
    void test_undoRedo_cutPattern_shouldUndoAndRedo();
    void test_undoRedo_cutSelection_shouldUndoAndRedo();
    void test_undoRedo_canUndoRedoSignals_shouldEmitSignals();
    void test_undoRedo_clearsOnNewSong_shouldClearStack();
    void test_undoRedo_patternChange_shouldNotClearStack();
    void test_undoRedo_shouldRestorePositionOnUndoRedo();
    void test_undoRedo_clearsOnStructuralChange_shouldClearStack();
    void test_undoRedo_transposeColumn_shouldUndoAndRedo();
    void test_undoRedo_transposeTrack_shouldUndoAndRedo();
    void test_undoRedo_transposePattern_shouldUndoAndRedo();
    void test_undoRedo_transposeSelection_shouldUndoAndRedo();
    void test_undoRedo_linearVelocityInterpolation_shouldUndoAndRedo();
};

} // namespace noteahead

#endif // EDITOR_SERVICE_UNDO_TEST_HPP
