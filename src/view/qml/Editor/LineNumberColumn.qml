import QtQuick 2.15
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
            if (listView) {
                listView.positionViewAtIndex(position.line, ListView.Beginning);
            }
        }
    }
    function updateData() {
    }
    function _lineHeight() {
        const lineCount = settingsService.visibleLines;
        return rootItem.height / lineCount;
    }
    ListView {
        id: listView
        anchors.fill: parent
        model: editorService.currentLineCount + settingsService.visibleLines
        delegate: LineNumberDelegate {
            width: rootItem.width
            height: _lineHeight()
            index: model.index
        }
        interactive: false
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