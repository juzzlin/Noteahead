import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnBackgroundColor
    clip: true
    signal leftClicked(int x, int y, int lineIndex)
    signal rightClicked(int x, int y, int lineIndex)
    property int _index: 0
    property int _patternIndex: 0
    property int _trackIndex: 0
    property int _scrollOffset: 0
    property Item _positionBar
    property var _lines: []
    readonly property string _tag: "NoteColumn"
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
        _lines.forEach((line, index) => {
                line.y = lineHeight * _scrolledLinePositionByLineIndex(index);
                line.width = width;
                line.height = lineHeight;
            });
    }
    function _scrollLines() {
        const lineHeight = _lineHeight();
        const linesVisible = config.visibleLines;
        _lines.forEach((line, index) => {
                const scrolledLinePosition = _scrolledLinePositionByLineIndex(index);
                line.y = lineHeight * scrolledLinePosition;
                line.visible = scrolledLinePosition >= 0 && scrolledLinePosition <= linesVisible;
            });
    }
    function _setLineFocused(lineIndex, lineColumnIndex, focused) {
        if (!focused) {
            lineCursor.visible = false;
        } else {
            _lines[lineIndex].setCursor(lineCursor, lineColumnIndex);
            lineCursor.visible = true;
        }
    }
    function _lineIndexAtPosition(y) {
        let bestIndex = 0;
        _lines.forEach((line, index) => {
                if (y >= line.y) {
                    bestIndex = index;
                }
            });
        return bestIndex;
    }
    function _updateIndexHighlights() {
        function _indexHighlightOpacity(index, linesPerBeat) {
            const _beatLine1 = linesPerBeat;
            const _beatLine2 = _beatLine1 % 3 ? _beatLine1 / 2 : _beatLine1 / 3;
            const _beatLine3 = _beatLine1 % 6 ? _beatLine1 / 4 : _beatLine1 / 6;
            if (!(index % _beatLine1))
                return 0.25;
            if (!(index % _beatLine3) && !(index % _beatLine2))
                return 0.10;
            if (!(index % _beatLine3))
                return 0.05;
            return 0;
        }
        function _scaledColor(opacity) {
            const value = Math.round(255 * opacity);
            return "#" + value.toString(16).padStart(2, "0").repeat(3);
        }
        _lines.forEach((line, index) => {
                line.color = _scaledColor(_indexHighlightOpacity(index, editorService.linesPerBeat));
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
    Rectangle {
        id: lineCursor
        color: "red"
        opacity: 0.5
        anchors.verticalCenter: parent.verticalCenter
        z: 3
        visible: false
    }
    VolumeMeter {
        id: volumeMeter
        anchors.top: rootItem.top
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        height: _positionBar ? _positionBar.y - rootItem.parent.y : 0
        z: 5
    }
    MouseArea {
        id: clickHandler
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: mouse => {
            const lineIndex = rootItem._lineIndexAtPosition(mouse.y);
            if (mouse.button === Qt.LeftButton) {
                uiLogger.debug(_tag, `Column ${rootItem._index} left clicked on line  ${lineIndex}`);
                rootItem.leftClicked(mouse.x, mouse.y, lineIndex);
            } else {
                uiLogger.debug(_tag, `Column ${rootItem._index} right clicked on line ${lineIndex}`);
                rootItem.rightClicked(mouse.x, mouse.y, lineIndex);
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
