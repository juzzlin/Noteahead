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
    function resize(width: int, height: int): void {
        rootItem.width = width;
        rootItem.height = height;
        lineContainer.width = width;
        lineContainer.height = height - columnHeader.height;
        _resizeLines();
    }
    function dataUpdated(): bool {
        return _dataUpdated;
    }
    function index(): int {
        return _index;
    }
    function setIndex(index: int): void {
        _index = index;
    }
    function setPatternIndex(index: int): void {
        _patternIndex = index;
    }
    function setTrackIndex(index: int): void {
        _trackIndex = index;
        columnHeader.setIndex(index);
    }
    function setFocused(focused: bool): void {
        _setLineFocused(editorService.position.line, editorService.position.lineColumn, focused);
    }
    function setName(name: string): void {
        columnHeader.setName(name);
    }
    function setMuted(muted: bool): void {
        columnHeader.setMuted(muted);
    }
    function setSoloed(soloed: bool): void {
        columnHeader.setSoloed(soloed);
    }
    function setVelocityScale(value: int): void {
        columnHeader.setVelocityScale(value);
    }
    function _triggerVolumeMeterAtPosition(position: var): void {
        if (UiService.isPlaying() && mixerService.shouldColumnPlay(_trackIndex, _index)) {
            const velocity = editorService.velocityAtPosition(position.pattern, _trackIndex, _index, position.line);
            volumeMeter.trigger(mixerService.effectiveVelocity(_trackIndex, _index, velocity) / 127);
        }
    }
    function setPosition(position: var): void {
        _scrollOffset = position.line;
        _scrollLines();
        _triggerVolumeMeterAtPosition(position);
    }
    function setPositionBar(positionBar: var): void {
        _positionBar = positionBar;
    }
    function updateData(): void {
        _createLines();
        _dataUpdated = true;
    }
    // Due to lazy loading, ensure that lines are created when requested
    function _getLineAtIndex(index: int): var {
        if (!_lines[index]) {
            _createLines();
        }
        return _lines[index];
    }
    function updateNoteDataAtPosition(position: var): void {
        if (_isPositionMe(position)) {
            const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, position.line);
            _getLineAtIndex(position.line).setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
        }
    }
    function _updateIndexHighlightsOnLine(line: NoteColumnLine, index: int): void {
        if (selectionService.isSelected(_patternIndex, _trackIndex, _index, index)) {
            const baseColor = utilService.scaledColor("#ffffff", utilService.indexHighlightOpacity(index, editorService.linesPerBeat));
            line.color = utilService.blendColors(baseColor, "#ffa500", 0.5); // Blend 50% with orange
            line.border.width = 0;
        } else {
            if (editorService.hasInstrumentSettings(_patternIndex, _trackIndex, _index, index)) {
                const baseColor = utilService.scaledColor("#ffffff", utilService.indexHighlightOpacity(index, editorService.linesPerBeat));
                line.color = utilService.blendColors(baseColor, "#3e65ff", 0.5); // Universal.Cobalt
                line.border.width = 1;
            } else if (automationService.hasAutomations(_patternIndex, _trackIndex, _index, index)) {
                const baseColor = utilService.scaledColor("#ffffff", utilService.indexHighlightOpacity(index, editorService.linesPerBeat));
                const automationWeight = automationService.automationWeight(_patternIndex, _trackIndex, _index, index);
                const automationColor = utilService.blendColors("#e51400", "#60a917", automationWeight); // Universal.Red -> Universal.Green
                line.color = utilService.blendColors(baseColor, automationColor, 0.75);
                line.border.width = 1;
            } else {
                line.color = utilService.scaledColor("#ffffff", utilService.indexHighlightOpacity(index, editorService.linesPerBeat));
                line.border.width = 1;
            }
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
    function _isPositionMe(position: var): bool {
        return position.pattern === _patternIndex && position.track === _trackIndex && position.column === _index;
    }
    function _lineHeight(): int {
        const lineCount = config.visibleLines;
        return lineContainer.height / lineCount;
    }
    function _scrolledLinePositionByLineIndex(lineIndex: int): int {
        return lineIndex - _scrollOffset + editorService.positionBarLine();
    }
    function _initializeWithData(lineCount: int, lineHeight: int): void {
        for (let lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            const noteAndVelocity = editorService.displayNoteAndVelocityAtPosition(_patternIndex, _trackIndex, _index, lineIndex);
            const line = noteColumnLineComponent.createObject(lineContainer);
            line.width = lineContainer.width;
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
            const line = noteColumnLineComponent.createObject(lineContainer);
            line.width = lineContainer.width;
            line.height = lineHeight;
            line.x = 0;
            line.y = lineHeight * _scrolledLinePositionByLineIndex(lineIndex);
            line.setNoteData(noteAndVelocity[0], noteAndVelocity[1]);
            _lines.push(line);
        }
    }
    function _createLines(): void {
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
    function _resizeLines(): void {
        const lineCount = editorService.lineCount(_patternIndex);
        const lineHeight = _lineHeight();
        _lines.forEach((line, index) => {
                line.y = lineHeight * _scrolledLinePositionByLineIndex(index);
                line.width = width;
                line.height = lineHeight;
            });
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
    function _setLineFocused(lineIndex: int, lineColumnIndex: int, focused: bool): void {
        if (!focused) {
            lineCursor.visible = false;
        } else {
            _getLineAtIndex(lineIndex).setCursor(lineCursor, lineColumnIndex);
            lineCursor.visible = true;
        }
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
            anchors.verticalCenter: parent ? parent.verticalCenter : null
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
        columnHeader.invertedMuteRequested.connect(() => mixerService.invertMutedColumns(_trackIndex, _index));
        columnHeader.invertedSoloRequested.connect(() => mixerService.invertSoloedColumns(_trackIndex, _index));
        columnHeader.muteRequested.connect(() => mixerService.muteColumn(_trackIndex, _index, true));
        columnHeader.soloRequested.connect(() => mixerService.soloColumn(_trackIndex, _index, true));
        columnHeader.unmuteRequested.connect(() => mixerService.muteColumn(_trackIndex, _index, false));
        columnHeader.unsoloRequested.connect(() => mixerService.soloColumn(_trackIndex, _index, false));
        columnHeader.nameChanged.connect(name => editorService.setColumnName(_trackIndex, _index, name));
        columnHeader.velocityScaleRequested.connect(() => UiService.requestColumnVelocityScaleDialog(_trackIndex, _index));
    }
}
