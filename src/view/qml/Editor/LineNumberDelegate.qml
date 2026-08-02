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

Rectangle {
    color: lineNumber < 0 || lineNumber >= editorService.currentLineCount ? "transparent" : Constants.lineNumberColumnCellBackgroundColor
    border.color: Constants.lineNumberColumnCellBorderColor
    border.width: 1
    property int index
    readonly property int lineNumber: index - editorService.positionBarLine()
    readonly property int _wrappedLineNumber: (lineNumber % editorService.currentLineCount + editorService.currentLineCount) % editorService.currentLineCount
    function _formattedLineNumber() {
        return _wrappedLineNumber < 10 ? `0${_wrappedLineNumber}` : _wrappedLineNumber;
    }
    Text {
        color: lineNumber < 0 || lineNumber >= editorService.currentLineCount ? Constants.lineNumberColumnOverflowTextColor : Constants.lineNumberColumnTextColor
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        text: _formattedLineNumber()
        anchors.centerIn: parent
    }
    IndexHighlight {
        anchors.fill: parent
        index: _wrappedLineNumber
    }
}
