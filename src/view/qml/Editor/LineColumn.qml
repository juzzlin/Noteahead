import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.lineNumberColumnBackgroundColor
    property int _index: 0
    property int _trackIndex: 0
    property var _lines: []
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        _resizeLines();
    }
    function setIndex(index) {
        _index = index;
    }
    function setTrackIndex(index) {
        _trackIndex = index;
    }
    function updateData() {
        _createLines();
    }
    function _createLines() {
        console.log(`Creating line number column for track ${_trackIndex}`);
        _lines = [];
        const lineCount = editorService.lineCount(editorService.currentPatternId());
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const lineHeight = rootItem.height / lineCount;
            const line = textComponent.createObject(rootItem, {
                    "index": lineIndex,
                    "width": rootItem.width,
                    "height": lineHeight,
                    "x": 0,
                    "y": lineHeight * lineIndex
                });
            _lines.push(line);
        }
    }
    function _resizeLines() {
        const lineCount = editorService.lineCount(editorService.currentPatternId());
        const lineHeight = rootItem.height / lineCount;
        console.log(`Resizing lines of line number column of track ${_trackIndex} to width = ${width}, height = ${lineHeight}`);
        _lines.forEach(line => {
                line.y = lineHeight * line.index;
                line.width = width;
                line.height = lineHeight;
            });
    }
    Component {
        id: textComponent
        Rectangle {
            color: Constants.lineNumberColumnCellBackgroundColor
            border.color: Constants.lineNumberColumnCellBorderColor
            border.width: 1
            property int index
            Text {
                color: Constants.lineNumberColumnTextColor
                font.bold: true
                text: index
                anchors.centerIn: parent
            }
            IndexHighlight {
                anchors.fill: parent
                index: parent.index
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
