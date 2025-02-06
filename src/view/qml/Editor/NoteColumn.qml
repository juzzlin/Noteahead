import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnBackgroundColor
    clip: true
    signal leftClicked(int x, int y)
    signal rightClicked(int x, int y)
    property int _index: 0
    property int _patternIndex: 0
    property int _trackIndex: 0
    property int _scrollOffset: 0
    property Item _positionBar
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
    function setPatternIndex(index) {
        _patternIndex = index;
    }
    function setTrackIndex(index) {
        _trackIndex = index;
    }
    function setFocused(focused) {
        _setLineFocused(editorService.position().line, editorService.position().lineColumn, focused);
    }
    function setPosition(position) {
        _scrollOffset = position.line;
        _scrollLines();
        if (UiService.isPlaying() && mixerService.shouldTrackPlay(_trackIndex)) {
            volumeMeter.trigger(editorService.effectiveVolumeAtPosition(position.pattern, _trackIndex, _index, position.line));
        }
    }
    function setPositionBar(positionBar) {
        _positionBar = positionBar;
    }
    function updateData() {
        _createLines();
    }
    function updateNoteDataAtPosition(position) {
        if (_isPositionMe(position)) {
            const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, position.line);
            _lines[position.line].setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
        }
    }
    function _isPositionMe(position) {
        return position.pattern === _patternIndex && position.track === _trackIndex && position.column === _index;
    }
    function _lineHeight() {
        const lineCount = config.visibleLines;
        return rootItem.height / lineCount;
    }
    function _scrolledLinePositionByLineIndex(lineIndex) {
        return lineIndex - _scrollOffset + editorService.positionBarLine();
    }
    function _initializeWithNoData(lineCount, lineHeight) {
        const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, 0);
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const line = noteColumnLineComponent.createObject(rootItem, {
                    "index": lineIndex,
                    "width": rootItem.width,
                    "height": lineHeight,
                    "x": 0,
                    "y": lineHeight * _scrolledLinePositionByLineIndex(lineIndex)
                });
            line.setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
            _lines.push(line);
        }
    }
    function _initializeWithData(lineCount, lineHeight) {
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, lineIndex);
            const line = noteColumnLineComponent.createObject(rootItem, {
                    "index": lineIndex,
                    "width": rootItem.width,
                    "height": lineHeight,
                    "x": 0,
                    "y": lineHeight * _scrolledLinePositionByLineIndex(lineIndex)
                });
            line.setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
            _lines.push(line);
        }
    }
    function _createLines() {
        _lines.forEach(line => {
                line.destroy();
            });
        _lines = [];
        const lineCount = editorService.lineCount(_patternIndex);
        const lineHeight = _lineHeight();
        if (editorService.hasData(_patternIndex, _trackIndex, _index)) {
            _initializeWithData(lineCount, lineHeight);
        } else {
            _initializeWithNoData(lineCount, lineHeight);
        }
        _updateIndexHighlights();
    }
    function _resizeLines() {
        const lineCount = editorService.lineCount(_patternIndex);
        const lineHeight = _lineHeight();
        _lines.forEach(line => {
                line.y = lineHeight * _scrolledLinePositionByLineIndex(line.index);
                line.resize(width, lineHeight);
            });
    }
    function _scrollLines() {
        const lineHeight = _lineHeight();
        const linesVisible = config.visibleLines;
        _lines.forEach(line => {
                const scrolledLinePosition = _scrolledLinePositionByLineIndex(line.index);
                line.y = lineHeight * scrolledLinePosition;
                line.visible = scrolledLinePosition >= 0 && scrolledLinePosition <= linesVisible;
            });
    }
    function _setLineFocused(lineIndex, lineColumnIndex, focused) {
        _lines.forEach((line, index) => {
                line.setFocused(focused && index === lineIndex, lineColumnIndex);
            });
    }
    function _updateIndexHighlights() {
        _lines.forEach(line => {
                line.updateIndexHighlight(editorService.linesPerBeat);
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
    VolumeMeter {
        id: volumeMeter
        anchors.top: rootItem.top
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        height: _positionBar ? _positionBar.y - rootItem.parent.y : 0
        z: 3
    }
    MouseArea {
        id: clickHandler
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            if (mouse.button === Qt.LeftButton) {
                uiLogger.debug(_tag, `Column ${rootItem._index} left clicked`);
                rootItem.leftClicked(mouse.x, mouse.y);
            } else {
                uiLogger.debug(_tag, `Column ${rootItem._index} right clicked`);
                rootItem.rightClicked(mouse.x, mouse.y);
            }
        }
        onWheel: event => {
            if (!UiService.isPlaying()) {
                if (event.angleDelta.y > 0) {
                    editorService.requestScroll(-1);
                    event.accepted = true;
                } else if (event.angleDelta.y < 0) {
                    editorService.requestScroll(1);
                    event.accepted = true;
                }
            }
        }
    }
    Component.onCompleted: {
        config.visibleLinesChanged.connect(_resizeLines);
        editorService.linesPerBeatChanged.connect(_updateIndexHighlights);
    }
}
