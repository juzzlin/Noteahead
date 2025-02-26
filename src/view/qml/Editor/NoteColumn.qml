import QtQuick 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    signal leftClicked(int x, int y, int lineIndex)
    signal rightClicked(int x, int y, int lineIndex)
    property int _index: 0
    property int _patternIndex: 0
    property int _trackIndex: 0
    property int _scrollOffset: 0
    property Item _positionBar
    property var _lines: []
    property bool _dataUpdated: false
    readonly property string _tag: "NoteColumn"
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        lineContainer.width = width;
        lineContainer.height = height - columnHeader.height;
        _resizeLines();
    }
    function dataUpdated() {
        return _dataUpdated;
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
        columnHeader.setIndex(index);
    }
    function setFocused(focused) {
        _setLineFocused(editorService.position.line, editorService.position.lineColumn, focused);
    }
    function setName(name) {
        columnHeader.setName(name);
    }
    function setMuted(muted) {
        columnHeader.setMuted(muted);
    }
    function setSoloed(soloed) {
        columnHeader.setSoloed(soloed);
    }
    function setVelocityScale(value) {
        columnHeader.setVelocityScale(value);
    }
    function _triggerVolumeMeterAtPosition(position) {
        if (UiService.isPlaying() && mixerService.shouldColumnPlay(_trackIndex, _index)) {
            const velocity = editorService.velocityAtPosition(position.pattern, _trackIndex, _index, position.line);
            volumeMeter.trigger(mixerService.effectiveVelocity(_trackIndex, _index, velocity) / 127);
        }
    }
    function setPosition(position) {
        _scrollOffset = position.line;
        _scrollLines();
        _triggerVolumeMeterAtPosition(position);
    }
    function setPositionBar(positionBar) {
        _positionBar = positionBar;
    }
    function updateData() {
        _createLines();
        _dataUpdated = true;
    }
    function updateNoteDataAtPosition(position) {
        if (_isPositionMe(position)) {
            const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, position.line);
            if (!_lines[position.line]) {
                _createLines();
            }
            _lines[position.line].setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
        }
    }
    function updateIndexHighlights() {
        uiLogger.debug(_tag, `Updating index highlights of track ${_trackIndex}, column ${_index}`);
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
                if (editorService.hasInstrumentSettings(_patternIndex, _trackIndex, _index, index)) {
                    line.color = Universal.color(Universal.Cobalt);
                } else {
                    line.color = _scaledColor(_indexHighlightOpacity(index, editorService.linesPerBeat));
                }
            });
    }
    function _isPositionMe(position) {
        return position.pattern === _patternIndex && position.track === _trackIndex && position.column === _index;
    }
    function _lineHeight() {
        const lineCount = config.visibleLines;
        return lineContainer.height / lineCount;
    }
    function _scrolledLinePositionByLineIndex(lineIndex) {
        return lineIndex - _scrollOffset + editorService.positionBarLine();
    }
    function _initializeWithNoData(lineCount, lineHeight) {
        const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, 0);
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const line = noteColumnLineComponent.createObject(lineContainer, {
                    "width": lineContainer.width,
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
            const line = noteColumnLineComponent.createObject(lineContainer, {
                    "width": lineContainer.width,
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
        _lines.length = 0;
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
    NoteColumnHeader {
        id: columnHeader
        anchors.top: parent.top
        width: parent.width
    }
    Rectangle {
        id: lineContainer
        anchors.top: columnHeader.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.noteColumnBackgroundColor
        clip: true
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
            anchors.top: lineContainer.top
            anchors.left: lineContainer.left
            anchors.right: lineContainer.right
            height: _positionBar ? _positionBar.y - lineContainer.parent.y - 2 * columnHeader.height : 0
            z: 5
        }
        MouseArea {
            id: clickHandler
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: mouse => {
                const lineIndex = rootItem._lineIndexAtPosition(mouse.y);
                if (mouse.button === Qt.LeftButton) {
                    uiLogger.debug(_tag, `Column ${lineContainer._index} left clicked on line  ${lineIndex}`);
                    rootItem.leftClicked(mouse.x, mouse.y, lineIndex);
                } else {
                    uiLogger.debug(_tag, `Column ${lineContainer._index} right clicked on line ${lineIndex}`);
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
    }
    Component.onCompleted: {
        config.visibleLinesChanged.connect(_resizeLines);
        columnHeader.muteRequested.connect(() => mixerService.muteColumn(_trackIndex, _index, true));
        columnHeader.soloRequested.connect(() => mixerService.soloColumn(_trackIndex, _index, true));
        columnHeader.unmuteRequested.connect(() => mixerService.muteColumn(_trackIndex, _index, false));
        columnHeader.unsoloRequested.connect(() => mixerService.soloColumn(_trackIndex, _index, false));
        columnHeader.nameChanged.connect(name => editorService.setColumnName(_trackIndex, _index, name));
        columnHeader.velocityScaleRequested.connect(() => UiService.requestColumnVelocityScaleDialog(_trackIndex, _index));
    }
}
