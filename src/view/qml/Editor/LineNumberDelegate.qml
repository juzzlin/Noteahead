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
