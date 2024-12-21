import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    property int _index: 0
    property string _name
    property bool _focused
    readonly property string _tag: "Track"
    signal clicked
    signal nameChanged(string name)
    function index() {
        return _index;
    }
    function resize(width, height) {
        uiLogger.debug(_tag, `Resizing track ${_index} to width = ${width}, height = ${height}`);
        rootItem.width = width;
        rootItem.height = height;
        columnContainer.resize(rootItem.width, rootItem.height - trackHeader.height);
    }
    function focused() {
        return _focused;
    }
    function setFocused(focused) {
        _focused = focused;
        columnContainer.setFocused(focused);
    }
    function setIndex(index) {
        _index = index;
    }
    function setName(name) {
        _name = name;
    }
    function setPosition(position) {
        columnContainer.setPosition(position);
    }
    function updateData() {
        uiLogger.debug(_tag, `Updating data for track ${_index}`);
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
        function setFocused(focused) {
            const columnIndex = 0;
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
                uiLogger.debug(_tag, `Creating note column ${col} for track ${_index}`);
                const noteColumn = noteColumnComponent.createObject(columnContainer);
                uiLogger.debug(_tag, `Column width: ${noteColumnWidth}, height: ${noteColumnHeight}`);
                noteColumn.width = noteColumnWidth;
                noteColumn.height = noteColumnHeight;
                noteColumn.x = _noteColumnX(col);
                noteColumn.setIndex(col);
                noteColumn.setTrackIndex(_index);
                noteColumn.updateData();
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
    TrackFocusThing {
        anchors.fill: parent
        visible: focused()
    }
    MouseArea {
        id: clickHandler
        anchors.top: trackHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        onClicked: {
            uiLogger.debug(_tag, `Track ${rootItem._index} clicked`);
            rootItem.clicked();
        }
        onWheel: event => {
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
