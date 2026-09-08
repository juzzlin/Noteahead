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

import QtQuick 2.15
import ".."

QtObject {
    id: rootItem
    signal editorFocusRequested
    signal contextMenuRequested(int x, int y)
    // The cell the drag started from. Both ends are kept here rather than read off the events,
    // because the press grabs the mouse for the column it happened in: every event after it names
    // that column no matter where the pointer has gone since.
    property var _selectionTrack: null
    property int _selectionStartColumn: 0
    property int _selectionStartLine: 0
    property int _selectionEndColumn: 0
    property int _selectionEndLine: 0
    property bool _isDragging: false
    // The column comes from the position rather than from the event, so that a drag leaves the
    // cursor where it was released instead of back where it was pressed.
    function handleLeftClicked(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        editorService.requestPosition(track.patternIndex(), track.index(), track.columnIndexAt(x), lineIndex, 0);
        editorFocusRequested();
    }
    function handleRightClicked(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        editorService.requestPosition(track.patternIndex(), track.index(), columnIndex, lineIndex, 0);
        editorFocusRequested();
        UiService.requestContextMenu(x, y);
    }
    function handleLeftPressed(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (!UiService.isPlaying()) {
            selectionService.clear();
            _selectionTrack = track;
            _selectionStartColumn = columnIndex;
            _selectionEndColumn = columnIndex;
            _selectionStartLine = _clampLine(track, lineIndex);
            _selectionEndLine = _selectionStartLine;
            _isDragging = true;
        }
    }
    function handleRightPressed(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
    }
    function handleLeftReleased(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (_isDragging) {
            _dragTo(x, lineIndex);
            _isDragging = false;
            _selectionTrack = null;
        }
    }
    function handleRightReleased(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
    }
    function handleMouseMoved(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (_isDragging) {
            _dragTo(x, lineIndex);
        }
    }
    // The empty area above the first line and below the last one still reports the line it would
    // be over, and so does a drag that has run off the editor altogether. Clamped, both reach the
    // end of the pattern instead of past it -- the same thing EditorService does with a position.
    function _clampLine(track: var, lineIndex: int): int {
        return Math.max(0, Math.min(editorService.lineCount(track.patternIndex()) - 1, lineIndex));
    }
    // A selection is only made once the drag has actually left the cell it started from: a plain
    // click is a cursor move and nothing more.
    function _dragTo(sceneX: int, lineIndex: int): void {
        const columnIndex = _selectionTrack.columnIndexAt(sceneX);
        const line = _clampLine(_selectionTrack, lineIndex);
        if (_selectionEndColumn === columnIndex && _selectionEndLine === line) {
            return;
        }
        _selectionEndColumn = columnIndex;
        _selectionEndLine = line;
        if (_selectionEndColumn === _selectionStartColumn && _selectionEndLine === _selectionStartLine) {
            return;
        }
        const patternIndex = _selectionTrack.patternIndex();
        const trackIndex = _selectionTrack.index();
        selectionService.requestSelectionStart(patternIndex, trackIndex, _selectionStartColumn, _selectionStartLine);
        selectionService.requestSelectionEnd(patternIndex, trackIndex, _selectionEndColumn, _selectionEndLine);
    }
}
