import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    property int _index: 0
    property int _patternIndex: 0
    property string _name
    property bool _focused
    property Item _positionBar
    readonly property string _tag: "Track"
    signal leftClicked(int columnIndex, int x, int y)
    signal rightClicked(int columnIndex, int x, int y)
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
        _name = name;
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
        onColumnDeletionRequested: editorService.requestColumnDeletion(_index)
        onNameChanged: name => rootItem.nameChanged(name)
        onNewColumnRequested: editorService.requestNewColumn(_index)
        onTrackSettingsDialogRequested: UiService.requestTrackSettingsDialog(_index)
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
            _noteColumns = [];
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
        function updateNoteDataAtPosition(position) {
            _noteColumns.forEach(noteColumn => {
                    noteColumn.updateNoteDataAtPosition(position);
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
            noteColumn.leftClicked.connect((x, y) => {
                    uiLogger.debug(_tag, `Track ${rootItem._index} left clicked`);
                    rootItem.leftClicked(noteColumn.index(), x, y);
                });
            noteColumn.rightClicked.connect((x, y) => {
                    uiLogger.debug(_tag, `Track ${rootItem._index} right clicked`);
                    rootItem.rightClicked(noteColumn.index(), x, y);
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
