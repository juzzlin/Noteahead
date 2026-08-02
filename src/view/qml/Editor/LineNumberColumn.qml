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
import Noteahead 1.0
import ".."

Rectangle {
    id: rootItem
    color: themeService.lineNumberColumnBackgroundColor
    clip: true
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
    }
    property int _currentLine: -1
    function setPosition(position) {
        if (_currentLine !== position.line) {
            _currentLine = position.line;
            if (renderer) {
                renderer.scrollOffset = position.line;
            }
        }
    }
    function updateData() {
    }
    LineNumberRenderer {
        id: renderer
        anchors.fill: parent
        visibleLines: settingsService.visibleLines
        currentLineCount: editorService.currentLineCount
        linesPerBeat: editorService.linesPerBeat
        positionBarLine: editorService.positionBarLine()
        scrollOffset: editorService.position.line
        backgroundColor: themeService.lineNumberColumnCellBackgroundColor
        textColor: themeService.accentColor
    }
    Rectangle {
        id: borderRectangle
        color: "transparent"
        border.color: themeService.lineNumberColumnBorderColor
        border.width: 1
        anchors.fill: parent
        z: 2
    }
}
