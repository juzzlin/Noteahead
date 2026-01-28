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
    void test_undoRedo_noteOn();
    void test_undoRedo_noteOff();
    void test_undoRedo_velocityChange();
    void test_undoRedo_deleteNote();
    void test_undoRedo_insertNote();
    void test_undoRedo_backspaceNote();
    void test_undoRedo_noteOffWithRedundantRemoval();
    void test_undoRedo_pasteColumn();
    void test_undoRedo_pasteTrack();
    void test_undoRedo_pastePattern();
    void test_undoRedo_pasteSelection();
    void test_undoRedo_cutColumn();
    void test_undoRedo_cutTrack();
    void test_undoRedo_cutPattern();
    void test_undoRedo_cutSelection();
    void test_undoRedo_canUndoRedoSignals();
    void test_undoRedo_clearsOnNewSong();
    void test_undoRedo_shouldNotClearOnPatternChange();
    void test_undoRedo_shouldRestorePosition();
    void test_undoRedo_clearsOnStructuralChange();
    void test_undoRedo_transposeColumn();
    void test_undoRedo_transposeTrack();
    void test_undoRedo_transposePattern();
    void test_undoRedo_transposeSelection();
    void test_undoRedo_linearVelocityInterpolation();
};

} // namespace noteahead

#endif // EDITOR_SERVICE_UNDO_TEST_HPP
