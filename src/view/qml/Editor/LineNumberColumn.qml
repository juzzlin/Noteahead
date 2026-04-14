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
