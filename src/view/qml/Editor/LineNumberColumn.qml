import QtQuick 2.15
import Noteahead 1.0
import ".."

Rectangle {
    id: rootItem
    color: Constants.lineNumberColumnBackgroundColor
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
        backgroundColor: Constants.lineNumberColumnCellBackgroundColor
        textColor: Constants.lineNumberColumnTextColor
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
