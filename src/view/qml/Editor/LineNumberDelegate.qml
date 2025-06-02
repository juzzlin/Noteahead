import QtQuick 2.15
import ".."

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
