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
    function index() {
        return _index;
    }
    function setIndex(index) {
        _index = index;
    }
    function setPatternIndex(index) {
        _patternIndex = index;
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
        columnContainer.resize(rootItem.width, rootItem.height - trackHeader.height);
    }
    function focused() {
        return _focused;
    }
    function setFocused(columnIndex, focused) {
        _focused = focused;
        trackHeader.setFocused(focused);
        columnContainer.setFocused(columnIndex, focused);
    }
    function setName(name) {
        trackHeader.setName(name);
    }
    function setPosition(position) {
        columnContainer.setPosition(position);
    }
    function setPositionBar(positionBar) {
        _positionBar = positionBar;
        columnContainer.setPositionBar(positionBar);
    }
    function updateData() {
        _clearColumns();
        _createColumns();
    }
    function updateNoteDataAtPosition(position) {
        columnContainer.updateNoteDataAtPosition(position);
    }
    function updateColumnVisibility() {
        columnContainer.updateVisibility();
    }
    function updateIndexHighlights() {
        columnContainer.updateIndexHighlights();
    }
    function addColumn() {
        columnContainer.addColumn();
    }
    function deleteColumn() {
        columnContainer.deleteColumn();
    }
    function _clearColumns() {
        columnContainer.clearColumns();
    }
    function _createColumns() {
        columnContainer.createColumns();
    }
    TrackHeader {
        id: trackHeader
        anchors.top: parent.top
        width: parent.width
        Component.onCompleted: {
            columnDeletionRequested.connect(() => editorService.requestColumnDeletion(_index));
            muteTrackRequested.connect(() => mixerService.muteTrack(_index, true));
            nameChanged.connect(name => rootItem.nameChanged(name));
            newColumnRequested.connect(() => editorService.requestNewColumn(_index));
            soloTrackRequested.connect(() => mixerService.soloTrack(_index, true));
            trackSettingsDialogRequested.connect(() => UiService.requestTrackSettingsDialog(_index));
            unmuteTrackRequested.connect(() => mixerService.muteTrack(_index, false));
            unsoloTrackRequested.connect(() => mixerService.soloTrack(_index, false));
            mixerService.trackMuted.connect((trackIndex, mute) => {
                    if (trackIndex === _index) {
                        setTrackMuted(mute);
                    }
                });
            mixerService.trackSoloed.connect((trackIndex, solo) => {
                    if (trackIndex === _index) {
                        setTrackSoloed(solo);
                    }
                });
        }
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
        function clearColumns() {
            _noteColumns.forEach(noteColumn => noteColumn.destry());
            _noteColumns.length = 0;
        }
        function createColumns() {
            _noteColumnCount = editorService.columnCount(_index);
            _createNoteColumns();
        }
        function addColumn() {
            _noteColumnCount = editorService.columnCount(_index);
            const noteColumn = _createNoteColumn(_noteColumnCount - 1);
            _noteColumns.push(noteColumn);
            _resize(width, height);
            updateIndexHighlights();
        }
        function deleteColumn() {
            _noteColumns.pop();
            _noteColumnCount = editorService.columnCount(_index);
            _resize(width, height);
        }
        function setFocused(columnIndex, focused) {
            _noteColumns[columnIndex].setFocused(focused);
        }
        function setPosition(position) {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.setPosition(position);
                });
        }
        function setPositionBar(positionBar) {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.setPositionBar(positionBar);
                });
        }
        function updateIndexHighlights() {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.updateIndexHighlights();
                });
        }
        function updateNoteDataAtPosition(position) {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.updateNoteDataAtPosition(position);
                });
        }
        function updateVisibility() {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.visible = editorService.isColumnVisible(_index, noteColumn.index());
                });
        }
        function _noteColumnX(index) {
            return _noteColumnWidth() * index;
        }
        function _noteColumnWidth() {
            return width / _noteColumnCount;
        }
        function _createNoteColumn(columnIndex) {
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
            noteColumn.updateData();
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
        function _createNoteColumns() {
            _noteColumns = [];
            const noteColumnWidth = _noteColumnWidth();
            const noteColumnHeight = columnContainer.height;
            for (let col = 0; col < _noteColumnCount; col++) {
                const noteColumn = _createNoteColumn(col);
                _noteColumns.push(noteColumn);
            }
        }
        function resize(width, height) {
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
}
