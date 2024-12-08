import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

Rectangle {
    id: rootItem
    color: _index % 2 === 0 ? "lightgray" : "darkgray"
    border.color: "black"
    border.width: 1
    property int _index: 0
    property string _name
    signal nameChanged(string name)
    function index() {
        return _index;
    }
    function resize(width, height) {
        console.log(`Resizing track ${_index} to width = ${width}, height = ${height}`);
        rootItem.width = width;
        rootItem.height = height;
        columnContainer.resize(rootItem.width, rootItem.height - trackHeader.height);
    }
    function setIndex(index) {
        _index = index;
    }
    function setName(name) {
        _name = name;
    }
    function updateData() {
        console.log(`Updating data for track ${_index}`);
        _clearColumns();
        _createColumns();
    }
    function _clearColumns() {
        columnContainer.clearColumns();
    }
    function _createColumns() {
        columnContainer.createColumns();
    }
    Rectangle {
        id: trackHeader
        anchors.top: parent.top
        height: 32
        width: parent.width
        TextField {
            text: _name
            placeholderText: qsTr("Track name")
            font.pixelSize: 24
            height: parent.height
            width: parent.width
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            padding: 0  // Remove default padding
            onTextChanged: rootItem.nameChanged(text)
        }
    }
    Rectangle {
        id: columnContainer
        anchors.top: trackHeader.bottom
        anchors.bottom: rootItem.bottom
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        color: "red"
        property int _noteColumnCount
        property var _noteColumns: []
        property var _lineColumn
        function clearColumns() {
            if (_lineColumn) {
                _lineColumn.destroy();
            }
            _noteColumns.forEach(column => column.destroy());
        }
        function createColumns() {
            _noteColumnCount = editorService.columnCount(_index);
            _createLineColumn();
            _createNoteColumns();
        }
        function _lineColumnX() {
            return 0;
        }
        function _lineColumnWidth() {
            return width * 0.25;
        }
        function _noteColumnX(index) {
            return _noteColumnWidth() * index + _lineColumnX() + _lineColumnWidth();
        }
        function _noteColumnWidth() {
            return width * 0.75 / _noteColumnCount;
        }
        function _createLineColumn() {
            const lineColumnWidth = _lineColumnWidth();
            const lineColumnHeight = columnContainer.height;
            const lineColumn = lineColumnComponent.createObject(columnContainer);
            console.log(`Line column width: ${lineColumnWidth}, height: ${lineColumnHeight}`);
            lineColumn.width = lineColumnWidth;
            lineColumn.height = lineColumnHeight;
            lineColumn.x = _lineColumnX();
            lineColumn.updateData();
            _lineColumn = lineColumn;
        }
        function _createNoteColumns() {
            _noteColumns = [];
            const noteColumnWidth = _noteColumnWidth();
            const noteColumnHeight = columnContainer.height;
            for (let col = 0; col < _noteColumnCount; col++) {
                console.log(`Creating note column ${col} for track ${_index}`);
                const noteColumn = noteColumnComponent.createObject(columnContainer);
                console.log(`Column width: ${noteColumnWidth}, height: ${noteColumnHeight}`);
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
            const lineColumnWidth = _lineColumnWidth();
            const lineColumnHeight = height;
            _lineColumn.resize(lineColumnWidth, lineColumnHeight);
        }
        Component {
            id: lineColumnComponent
            LineColumn {
            }
        }
        Component {
            id: noteColumnComponent
            NoteColumn {
            }
        }
    }
}
