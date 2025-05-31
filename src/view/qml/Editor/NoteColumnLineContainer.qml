import QtQuick 2.15
import QtQuick.Controls.Universal 2.15
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
    readonly property string _tag: "NoteColumnLineContainer"
    property var _lines: []
    function setLocation(patternIndex: int, trackIndex: int, columnIndex: int): void {
        _patternIndex = patternIndex;
        _trackIndex = trackIndex;
        _index = columnIndex;
    }
    function createLines(): void {
        uiLogger.trace(_tag, `Creating lines of pattern ${_patternIndex}, track ${_trackIndex}, column ${_index}`);
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
        rootItem.updateIndexHighlights();
    }
    function setLineFocused(lineIndex: int, lineColumnIndex: int, focused: bool): void {
        if (!focused) {
            lineCursor.visible = false;
        } else {
            _getLineAtIndex(lineIndex).setCursor(lineCursor, lineColumnIndex);
            lineCursor.visible = true;
        }
    }
    function resizeLines(): void {
        const lineCount = editorService.lineCount(_patternIndex);
        const lineHeight = _lineHeight();
        _lines.forEach((line, index) => {
                line.y = lineHeight * _scrolledLinePositionByLineIndex(index);
                line.width = width;
                line.height = lineHeight;
            });
    }
    function updateNoteDataAtPosition(position: var): void {
        if (_isPositionMe(position)) {
            const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, position.line);
            _getLineAtIndex(position.line).setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
        }
    }
    function updateIndexHighlights(): void {
        _lines.forEach((line, index) => {
                _updateIndexHighlightsOnLine(line, index);
            });
    }
    function updateIndexHighlightsAtPosition(position: var): void {
        const line = _getLineAtIndex(position.line);
        if (line) {
            _updateIndexHighlightsOnLine(line, position.line);
        }
    }
    function setPosition(position: var): void {
        _scrollOffset = position.line;
        _scrollLines();
        _triggerVolumeMeterAtPosition(position);
    }
    // Due to lazy loading, ensure that lines are created when requested
    function _getLineAtIndex(index: int): var {
        if (!_lines[index]) {
            createLines();
        }
        return _lines[index];
    }
    function _scrollLines(): void {
        const lineHeight = _lineHeight();
        const linesVisible = config.visibleLines;
        _lines.forEach((line, index) => {
                const scrolledLinePosition = _scrolledLinePositionByLineIndex(index);
                line.y = lineHeight * scrolledLinePosition;
                line.visible = scrolledLinePosition >= 0 && scrolledLinePosition <= linesVisible;
            });
    }
    function _lineIndexAtPosition(y: int): int {
        let bestIndex = 0;
        _lines.forEach((line, index) => {
                if (y >= line.y) {
                    bestIndex = index;
                }
            });
        return bestIndex;
    }
    function _lineHeight(): int {
        const lineCount = config.visibleLines;
        return rootItem.height / lineCount;
    }
    function _scrolledLinePositionByLineIndex(lineIndex: int): int {
        return lineIndex - _scrollOffset + editorService.positionBarLine();
    }
    function _initializeWithData(lineCount: int, lineHeight: int): void {
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, lineIndex);
            const line = noteColumnLineComponent.createObject(rootItem);
            line.width = rootItem.width;
            line.height = lineHeight;
            line.x = 0;
            line.y = lineHeight * _scrolledLinePositionByLineIndex(lineIndex);
            line.setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
            _lines.push(line);
        }
    }
    function _initializeWithNoData(lineCount: int, lineHeight: int): void {
        const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, 0);
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const line = noteColumnLineComponent.createObject(rootItem);
            line.width = rootItem.width;
            line.height = lineHeight;
            line.x = 0;
            line.y = lineHeight * _scrolledLinePositionByLineIndex(lineIndex);
            line.setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
            _lines.push(line);
        }
    }
    function _isPositionMe(position: var): bool {
        return position.pattern === _patternIndex && position.track === _trackIndex && position.column === _index;
    }
    function _triggerVolumeMeterAtPosition(position: var): void {
        if (UiService.isPlaying() && mixerService.shouldColumnPlay(_trackIndex, _index)) {
            const velocity = editorService.velocityAtPosition(position.pattern, _trackIndex, _index, position.line);
            volumeMeter.trigger(mixerService.effectiveVelocity(_trackIndex, _index, velocity) / 127);
        }
    }
    function _updateIndexHighlightsOnLine(line: NoteColumnLine, index: int): void {
        const colorAndBorder = noteColumnLineContainerHelper.lineColorAndBorderWidth(_patternIndex, _trackIndex, _index, index);
        line.color = colorAndBorder[0];
        line.border.width = colorAndBorder[1];
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
        anchors.verticalCenter: parent ? parent.verticalCenter : null
        z: 3
        visible: false
    }
    VolumeMeter {
        id: volumeMeter
        anchors.top: rootItem.top
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        height: _positionBar ? _positionBar.y - rootItem.parent.y - 2 * columnHeader.height : 0
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
}
