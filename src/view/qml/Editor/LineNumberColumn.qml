import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.lineNumberColumnBackgroundColor
    property var _lines: []
    property int _scrollOffset: 0
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        _resizeLines();
    }
    function setPosition(position) {
        _lines.forEach(line => {
            line.updateLineNumber();
        });
    }
    function updateData() {
        _createLines();
    }
    function _lineHeight() {
        const lineCount = editorService.linesVisible();
        return rootItem.height / lineCount;
    }
    function _createLines() {
        _lines = [];
        const lineCount = editorService.lineCount(editorService.currentPattern);
        const lineHeight = _lineHeight();
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const line = textComponent.createObject(rootItem, {
                "index": lineIndex,
                "lineNumber": editorService.lineNumberAtViewLine(lineIndex),
                "width": rootItem.width,
                "height": lineHeight,
                "x": 0,
                "y": lineHeight * lineIndex
            });
            _lines.push(line);
        }
    }
    function _resizeLines() {
        const lineCount = editorService.currentLineCount;
        const lineHeight = _lineHeight();
        _lines.forEach(line => {
            line.y = lineHeight * (line.index + _scrollOffset);
            line.width = width;
            line.height = lineHeight;
        });
    }
    Component {
        id: textComponent
        Rectangle {
            color: lineNumber < 0 ? "transparent" : Constants.lineNumberColumnCellBackgroundColor
            border.color: Constants.lineNumberColumnCellBorderColor
            border.width: 1
            property int index
            property int lineNumber
            function updateLineNumber() {
                lineNumber = editorService.lineNumberAtViewLine(index);
            }
            function _formattedLineNumber() {
                const lineCount = editorService.currentLineCount;
                const formattedLineNumber = Math.abs(lineNumber) % lineCount;
                return formattedLineNumber < 10 ? `0${formattedLineNumber}` : formattedLineNumber;
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
                index: lineNumber
            }
        }
    }
    Rectangle {
        id: borderRectangle
        color: "transparent"
        border.color: Constants.lineNumberColumnBorderColor
        border.width: 1
        anchors.fill: parent
        z: 2
    }
}
