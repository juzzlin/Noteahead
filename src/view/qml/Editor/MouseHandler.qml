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
    property int _selectionStartLine: 0
    property int _selectionEndLine: 0
    property bool _isDragging: false
    function handleLeftClicked(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        editorService.requestPosition(track.patternIndex(), track.index(), columnIndex, lineIndex, 0);
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
            _selectionStartLine = lineIndex;
            _selectionEndLine = lineIndex;
            _isDragging = true;
        }
    }
    function handleRightPressed(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
    }
    function handleLeftReleased(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (!UiService.isPlaying()) {
            selectionService.requestSelectionEnd(editorService.position.pattern, track.index(), columnIndex, _selectionEndLine);
            _isDragging = false;
        }
    }
    function handleRightReleased(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
    }
    function handleMouseMoved(track: var, columnIndex: int, lineIndex: int, x: int, y: int): void {
        if (!UiService.isPlaying()) {
            if (_isDragging) {
                if (_selectionEndLine !== lineIndex) {
                    _selectionEndLine = lineIndex;
                    selectionService.requestSelectionEnd(editorService.position.pattern, track.index(), columnIndex, _selectionEndLine);
                }
                if (_selectionStartLine !== _selectionEndLine) {
                    selectionService.requestSelectionStart(editorService.position.pattern, track.index(), columnIndex, _selectionStartLine);
                }
            }
        }
    }
}
