import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Rectangle {
    id: rootItem
    color: "black"
    property int _index: 0
    property int _patternIndex: 0
    property bool _focused
    property Item _positionBar
    property string _name
    readonly property string _tag: "Track"
    signal leftClicked(int columnIndex, int lineIndex, int x, int y)
    signal rightClicked(int columnIndex, int lineIndex, int x, int y)
    signal leftPressed(int columnIndex, int lineIndex, int x, int y)
    signal rightPressed(int columnIndex, int lineIndex, int x, int y)
    signal leftReleased(int columnIndex, int lineIndex, int x, int y)
    signal rightReleased(int columnIndex, int lineIndex, int x, int y)
    signal mouseMoved(int columnIndex, int lineIndex, int x, int y)
    signal nameChanged(string name)
    function index(): int {
        return _index;
    }
    function patternIndex(): int {
        return _patternIndex;
    }
    function setLocation(patternIndex: int, trackIndex: int): void {
        _patternIndex = patternIndex;
        _index = trackIndex;
        _initializeTrackHeader();
    }
    function _initializeTrackHeader(): void {
        trackHeader.setIndex(_index);
        setMuted(mixerService.isTrackMuted(_index));
        setSoloed(mixerService.isTrackSoloed(_index));
        setVelocityScale(mixerService.trackVelocityScale(_index));
    }
    function setDimensions(trackAreaWidth: int, trackAreaHeight: int, unitWidth: double): void {
        resize(unitWidth * editorService.trackWidthInUnits(index()), trackAreaHeight);
        const newX = unitWidth * editorService.onScreenTrackPositionInUnits(index());
        const newY = 0;
        if (x !== newX || y !== newY) {
            x = newX;
            y = newY;
        }
    }
    function resize(width: int, height: int): void {
        rootItem.width = width;
        rootItem.height = height;
        columnContainer.resize(rootItem.width, rootItem.height - trackHeader.height);
    }
    function focused(): bool {
        return _focused;
    }
    function setFocused(columnIndex: int, focused: bool): void {
        _focused = focused;
        trackHeader.setFocused(focused);
    }
    function setName(name: string): void {
        trackHeader.setName(name);
    }
    function setPosition(position: var): void {
        columnContainer.setPosition(position);
    }
    function setPositionBar(positionBar: var): void {
        _positionBar = positionBar;
        columnContainer.setPositionBar(positionBar);
    }
    function setMuted(muted: bool): void {
        trackHeader.setMuted(muted);
    }
    function setSoloed(soloed: bool): void {
        trackHeader.setSoloed(soloed);
    }
    function setVelocityScale(value: int): void {
        trackHeader.setVelocityScale(value);
    }
    function setColumnMuted(columnIndex: int, muted: bool): void {
        columnContainer.setColumnMuted(columnIndex, muted);
    }
    function setColumnSoloed(columnIndex: int, soloed: bool): void {
        columnContainer.setColumnSoloed(columnIndex, soloed);
    }
    function setColumnVelocityScale(columnIndex: int, value: int): void {
        columnContainer.setColumnVelocityScale(columnIndex, value);
    }
    function clearMixerSettings(): void {
        setMuted(false);
        setSoloed(false);
        setVelocityScale(100);
        columnContainer.clearMixerSettings();
    }
    function updateData(): void {
        _clearColumns();
        _createColumns();
    }
    function updateNoteDataAtPosition(position: var): void {
        columnContainer.updateNoteDataAtPosition(position);
    }
    function updateColumnHeaders(): void {
        columnContainer.updateColumnHeaders();
    }
    function addColumn(): void {
        columnContainer.addColumn();
    }
    function deleteColumn(): void {
        columnContainer.deleteColumn();
    }
    function _clearColumns(): void {
        columnContainer.clearColumns();
    }
    function _createColumns(): void {
        columnContainer.createColumns();
    }
    TrackHeader {
        id: trackHeader
        anchors.top: parent.top
        width: parent.width
    }
    Item {
        id: columnContainer
        anchors.top: trackHeader.bottom
        anchors.bottom: rootItem.bottom
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        property int _noteColumnCount
        property var _noteColumns: []
        property var _lineColumn
        function clearColumns(): void {
            _noteColumns.forEach(noteColumn => noteColumn.destroy());
            _noteColumns.length = 0;
        }
        function createColumns(): void {
            _noteColumnCount = editorService.columnCount(_index);
            _createNoteColumns();
        }
        function addColumn(): void {
            _noteColumnCount = editorService.columnCount(_index);
            const noteColumn = _createNoteColumn(_noteColumnCount - 1);
            _noteColumns.push(noteColumn);
            noteColumn.setPosition(editorService.position);
            resize(rootItem.width, rootItem.height);
        }
        function deleteColumn(): void {
            _noteColumns[_noteColumns.length - 1].destroy();
            _noteColumns.pop();
            _noteColumnCount = editorService.columnCount(_index);
            resize(rootItem.width, rootItem.height);
        }
        function setColumnMuted(columnIndex: int, muted: bool): void {
            _noteColumns[columnIndex].setMuted(muted);
        }
        function setColumnSoloed(columnIndex: int, soloed: bool): void {
            _noteColumns[columnIndex].setSoloed(soloed);
        }
        function setColumnVelocityScale(columnIndex: int, value: int): void {
            _noteColumns[columnIndex].setVelocityScale(value);
        }
        function clearMixerSettings(): void {
            _noteColumns.forEach(noteColumn => {
                noteColumn.setMuted(false);
                noteColumn.setSoloed(false);
                noteColumn.setVelocityScale(100);
            });
        }
        function setPosition(position: var): void {
            _noteColumns.forEach(noteColumn => {
                if (editorService.isColumnVisible(_index, noteColumn.index())) {
                    noteColumn.setPosition(position);
                }
            });
        }
        function setPositionBar(positionBar: var): void {
            _noteColumns.forEach(noteColumn => {
                noteColumn.setPositionBar(positionBar);
            });
        }
        function updateColumnHeaders(): void {
            _noteColumns.forEach(noteColumn => {
                noteColumn.setName(editorService.columnName(_index, noteColumn.index()));
                noteColumn.setMuted(mixerService.isColumnMuted(_index, noteColumn.index()));
                noteColumn.setSoloed(mixerService.isColumnSoloed(_index, noteColumn.index()));
                noteColumn.setVelocityScale(mixerService.columnVelocityScale(_index, noteColumn.index()));
            });
        }
        function _noteColumnX(index: int): int {
            return _noteColumnWidth() * index;
        }
        function _noteColumnWidth(): int {
            return width / _noteColumnCount;
        }
        function _createNoteColumn(columnIndex: int): var {
            const noteColumnWidth = _noteColumnWidth();
            const noteColumnHeight = columnContainer.height;
            const noteColumn = noteColumnComponent.createObject(columnContainer);
            noteColumn.width = noteColumnWidth;
            noteColumn.height = noteColumnHeight;
            noteColumn.x = _noteColumnX(columnIndex);
            noteColumn.setLocation(_patternIndex, _index, columnIndex);
            noteColumn.setPositionBar(_positionBar);
            noteColumn.leftClicked.connect((lineIndex, x, y) => {
                uiLogger.debug(_tag, `Track ${rootItem._index} left clicked`);
                rootItem.leftClicked(noteColumn.index(), lineIndex, x + rootItem.x, y + rootItem.y);
            });
            noteColumn.rightClicked.connect((lineIndex, x, y) => {
                uiLogger.debug(_tag, `Track ${rootItem._index} right clicked`);
                rootItem.rightClicked(noteColumn.index(), lineIndex, x, y);
            });
            noteColumn.leftPressed.connect((lineIndex, x, y) => {
                uiLogger.debug(_tag, `Track ${rootItem._index} left pressed`);
                rootItem.leftPressed(noteColumn.index(), lineIndex, x, y);
            });
            noteColumn.rightPressed.connect((lineIndex, x, y) => {
                uiLogger.debug(_tag, `Track ${rootItem._index} right pressed`);
                rootItem.rightPressed(noteColumn.index(), lineIndex, x, y);
            });
            noteColumn.leftReleased.connect((lineIndex, x, y) => {
                uiLogger.debug(_tag, `Track ${rootItem._index} left released`);
                rootItem.leftReleased(noteColumn.index(), lineIndex, x, y);
            });
            noteColumn.rightReleased.connect((lineIndex, x, y) => {
                uiLogger.debug(_tag, `Track ${rootItem._index} right released`);
                rootItem.rightReleased(noteColumn.index(), lineIndex, x, y);
            });
            noteColumn.mouseMoved.connect((lineIndex, x, y) => {
                uiLogger.debug(_tag, `Track ${rootItem._index} mouse moved`);
                rootItem.mouseMoved(noteColumn.index(), lineIndex, x, y);
            });
            return noteColumn;
        }
        function _createNoteColumns(): void {
            _noteColumns = [];
            const noteColumnWidth = _noteColumnWidth();
            const noteColumnHeight = columnContainer.height;
            for (let col = 0; col < _noteColumnCount; col++) {
                const noteColumn = _createNoteColumn(col);
                _noteColumns.push(noteColumn);
            }
        }
        function resize(width: int, height: int): void {
            columnContainer.width = width;
            columnContainer.height = height;
            const noteColumnWidth = _noteColumnWidth();
            const noteColumnHeight = height;
            _noteColumns.forEach(noteColumn => {
                noteColumn.x = _noteColumnX(noteColumn.index());
                noteColumn.resize(noteColumnWidth, noteColumnHeight);
            });
        }
        Component {
            id: noteColumnComponent
            NoteColumn {}
        }
    }
    Item {
        id: trackBorder
        anchors.fill: parent
        z: 5
        Rectangle {
            id: borderFocusedL
            color: Constants.trackHeaderTextColor(rootItem._index)
            width: Constants.trackBorderFocusedWidth
            height: parent.height
            anchors.top: parent.top
            anchors.left: parent.left
            visible: rootItem._focused
        }
        Rectangle {
            id: borderFocusedR
            color: Constants.trackHeaderTextColor(rootItem._index)
            width: Constants.trackBorderFocusedWidth
            height: parent.height
            anchors.top: parent.top
            anchors.right: parent.right
            visible: rootItem._focused
        }
    }
    Component.onCompleted: {
        trackHeader.columnDeletionRequested.connect(() => editorService.requestColumnDeletion(_index));
        trackHeader.invertedMuteRequested.connect(() => mixerService.invertMutedTracks(_index));
        trackHeader.invertedSoloRequested.connect(() => mixerService.invertSoloedTracks(_index));
        trackHeader.muteRequested.connect(() => mixerService.muteTrack(_index, true));
        trackHeader.nameChanged.connect(name => rootItem.nameChanged(name));
        trackHeader.newColumnRequested.connect(() => editorService.requestNewColumn(_index));
        trackHeader.soloRequested.connect(() => mixerService.soloTrack(_index, true));
        trackHeader.trackSettingsDialogRequested.connect(() => UiService.requestTrackSettingsDialog(_index));
        trackHeader.unmuteRequested.connect(() => mixerService.muteTrack(_index, false));
        trackHeader.unsoloRequested.connect(() => mixerService.soloTrack(_index, false));
        trackHeader.velocityScaleRequested.connect(() => UiService.requestTrackVelocityScaleDialog(_index));
    }
}
