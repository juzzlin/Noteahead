import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnBackgroundColor
    clip: true
    property int _index: 0
    property int _trackIndex: 0
    property int _scrollOffset: 0
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
    function setFocused(focused) {
        const lineIndex = editorService.position().line;
        _setLineFocused(lineIndex, focused);
    }
    function setPosition(position) {
        _scrollOffset = position.line;
        _scrollLines();
    }
    function updateData() {
        _createLines();
    }
    function _lineHeight() {
        const lineCount = editorService.linesVisible();
        return rootItem.height / lineCount;
    }
    function _scrolledLinePositionByLineIndex(lineIndex) {
        return lineIndex - _scrollOffset + editorService.positionBarLine();
    }
    function _createLines() {
        _lines = [];
        const lineCount = editorService.currentLineCount();
        const lineHeight = _lineHeight();
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const note = editorService.noteAtPosition(editorService.currentPatternId(), _trackIndex, _index, lineIndex);
            const line = noteColumnLineComponent.createObject(rootItem, {
                    "index": lineIndex,
                    "width": rootItem.width,
                    "height": lineHeight,
                    "x": 0,
                    "y": lineHeight * _scrolledLinePositionByLineIndex(lineIndex),
                    "note": note
                });
            _lines.push(line);
        }
    }
    function _resizeLines() {
        const lineCount = editorService.currentLineCount();
        const lineHeight = _lineHeight();
        _lines.forEach(line => {
                line.y = lineHeight * _scrolledLinePositionByLineIndex(line.index);
                line.resize(width, lineHeight);
            });
    }
    function _scrollLines() {
        const lineHeight = _lineHeight();
        _lines.forEach(line => {
                line.y = lineHeight * _scrolledLinePositionByLineIndex(line.index);
            });
    }
    function _setLineFocused(lineIndex, focused) {
        console.log("Setting line " + lineIndex + " focused: " + focused);
        _lines.forEach((line, index) => {
                line.setFocused(focused && index === lineIndex);
            });
    }
    Component {
        id: noteColumnLineComponent
        NoteColumnLine {
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
