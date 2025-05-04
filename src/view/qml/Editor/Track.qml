import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    property int _index: 0
    property int _patternIndex: 0
    property bool _focused
    property Item _positionBar
    property string _name
    readonly property string _tag: "Track"
    signal leftClicked(int columnIndex, int lineIndex, int x, int y)
    signal rightClicked(int columnIndex, int lineIndex, int x, int y)
    signal nameChanged(string name)
    function index(): int {
        return _index;
    }
    function setIndex(index: int): void {
        _index = index;
        trackHeader.setIndex(index);
    }
    function setPatternIndex(index: int): void {
        _patternIndex = index;
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
        columnContainer.setFocused(columnIndex, focused);
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
    function updateColumnVisibility(): void {
        columnContainer.updateVisibility();
    }
    function updateIndexHighlights(): void {
        columnContainer.updateIndexHighlights();
    }
    function updateIndexHighlightsAtPosition(position: var): void {
        columnContainer.updateIndexHighlightsAtPosition(position);
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
            _resize(width, height);
            updateIndexHighlights();
        }
        function deleteColumn(): void {
            _noteColumns[_noteColumns.length - 1].destroy();
            _noteColumns.pop();
            _noteColumnCount = editorService.columnCount(_index);
            _resize(width, height);
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
        function setFocused(columnIndex: int, focused: bool): void {
            _noteColumns[columnIndex].setFocused(focused);
        }
        function setPosition(position: var): void {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.setPosition(position);
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
        function updateIndexHighlights(): void {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.updateIndexHighlights();
                });
        }
        function updateIndexHighlightsAtPosition(position: var): void {
            _noteColumns.filter(noteColumn => noteColumn.index() === position.column).forEach(noteColumn => {
                    noteColumn.updateIndexHighlightsAtPosition(position);
                });
        }
        function updateNoteDataAtPosition(position: var): void {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.updateNoteDataAtPosition(position);
                });
        }
        function updateVisibility(): void {
            // Load column data if the column becomes visible or is visible but not yet loaded
            _noteColumns.forEach(noteColumn => {
                    if (!noteColumn.dataUpdated() && editorService.isColumnVisible(_index, noteColumn.index())) {
                        noteColumn.updateData();
                    }
                });
        }
        function _noteColumnX(index: int): int {
            return _noteColumnWidth() * index;
        }
        function _noteColumnWidth(): int {
            return width / _noteColumnCount;
        }
        // Note!!: The actual line data objects are not yet created, but on-demand when updateVisibility() is called.
        //         This will significantly reduce the load time of a song while providing a smooth playback animation.
        function _createNoteColumn(columnIndex: int): var {
            const noteColumnWidth = _noteColumnWidth();
            const noteColumnHeight = columnContainer.height;
            const noteColumn = noteColumnComponent.createObject(columnContainer);
            noteColumn.width = noteColumnWidth;
            noteColumn.height = noteColumnHeight;
            noteColumn.x = _noteColumnX(columnIndex);
            noteColumn.setIndex(columnIndex);
            noteColumn.setTrackIndex(_index);
            noteColumn.setPatternIndex(_patternIndex);
            noteColumn.setPositionBar(_positionBar);
            noteColumn.leftClicked.connect((x, y, lineIndex) => {
                    uiLogger.debug(_tag, `Track ${rootItem._index} left clicked`);
                    rootItem.leftClicked(noteColumn.index(), lineIndex, x, y);
                });
            noteColumn.rightClicked.connect((x, y, lineIndex) => {
                    uiLogger.debug(_tag, `Track ${rootItem._index} right clicked`);
                    rootItem.rightClicked(noteColumn.index(), lineIndex, x, y);
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
            NoteColumn {
            }
        }
    }
    Rectangle {
        id: borderRectangle
        anchors.fill: parent
        color: "transparent"
        border.color: Constants.trackBorderColor
        border.width: Constants.trackBorderWidth
    }
    Component.onCompleted: {
        trackHeader.columnDeletionRequested.connect(() => editorService.requestColumnDeletion(_index));
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
