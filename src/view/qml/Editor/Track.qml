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
    signal clicked(int columnIndex)
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
        if (UiService.isPlaying()) {
            volumeMeter.trigger(editorService.effectiveVolumeAtPosition(position.pattern, _index, position.column, position.line));
        }
    }
    function setPositionBar(positionBar) {
        _positionBar = positionBar;
    }
    function setPositionBarY(positionBarY) {
        _positionBarY = positionBarY;
    }
    function updateData() {
        _clearColumns();
        _createColumns();
    }
    function updateNoteDataAtPosition(position) {
        columnContainer.updateNoteDataAtPosition(position);
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
        onNameChanged: name => rootItem.nameChanged(name)
        onNewColumnRequested: editorService.requestNewColumn(_index)
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
            _noteColumns.forEach(column => column.destroy());
        }
        function createColumns() {
            _noteColumnCount = editorService.columnCount(_index);
            _createNoteColumns();
        }
        function setFocused(columnIndex, focused) {
            _noteColumns[columnIndex].setFocused(focused);
        }
        function setPosition(position) {
            _noteColumns.forEach(noteColumn => {
                noteColumn.setPosition(position);
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
        function _createNoteColumns() {
            _noteColumns = [];
            const noteColumnWidth = _noteColumnWidth();
            const noteColumnHeight = columnContainer.height;
            for (let col = 0; col < _noteColumnCount; col++) {
                const noteColumn = noteColumnComponent.createObject(columnContainer);
                noteColumn.width = noteColumnWidth;
                noteColumn.height = noteColumnHeight;
                noteColumn.x = _noteColumnX(col);
                noteColumn.setIndex(col);
                noteColumn.setTrackIndex(_index);
                noteColumn.setPatternIndex(_patternIndex);
                noteColumn.updateData();
                noteColumn.clicked.connect(() => {
                    uiLogger.debug(_tag, `Track ${rootItem._index} clicked`);
                    rootItem.clicked(noteColumn.index());
                });
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
    VolumeMeter {
        id: volumeMeter
        anchors.top: trackHeader.bottom
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        height: _positionBar ? _positionBar.y - y : 0
    }
    Rectangle {
        id: borderRectangle
        anchors.fill: parent
        color: "transparent"
        border.color: Constants.trackBorderColor
        border.width: Constants.trackBorderWidth
    }
}
