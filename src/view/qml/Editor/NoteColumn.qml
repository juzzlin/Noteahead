import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnBackgroundColor
    property int _index: 0
    property int _trackIndex: 0
    property var _lines: []
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        _resizeLines();
    }
    function index() {
        return _index;
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
        console.log(`Creating lines for column ${_index} on track ${_trackIndex}`);
        _lines = [];
        const lineCount = editorService.lineCount(editorService.currentPatternId());
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const note = editorService.noteAtPosition(editorService.currentPatternId(), _trackIndex, _index, lineIndex);
            const lineHeight = rootItem.height / lineCount;
            const line = lineComponent.createObject(rootItem, {
                    "index": lineIndex,
                    "width": rootItem.width,
                    "height": lineHeight,
                    "x": 0,
                    "y": lineHeight * lineIndex,
                    "note": note
                });
            _lines.push(line);
        }
    }
    function _resizeLines() {
        const lineCount = editorService.lineCount(editorService.currentPatternId());
        const lineHeight = rootItem.height / lineCount;
        console.log(`Resizing lines of column ${_trackIndex}, ${_index} to width = ${width}, height = ${lineHeight}`);
        _lines.forEach(line => {
                line.y = lineHeight * line.index;
                line.resize(width, lineHeight);
            });
    }
    Component {
        id: lineComponent
        Line {
        }
    }
    Rectangle {
        id: borderRectangle
        color: "transparent"
        border.color: Constants.noteColumnBorderColor
        border.width: 1
        anchors.fill: parent
        z: 2
    }
}
