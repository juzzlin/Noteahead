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
    property var _columns: []
    signal nameChanged(string name)
    function index() {
        return _index;
    }
    function resize(width, height) {
        console.log(`Resizing track ${_index} to width = ${width}, height = ${height}`);
        rootItem.width = width;
        rootItem.height = height;
        noteColumnContainer.resize(rootItem.width, rootItem.height);
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
        _columns.forEach(column => column.destroy());
    }
    function _createColumns() {
        noteColumnContainer.createColumns();
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
        id: noteColumnContainer
        anchors.top: trackHeader.bottom
        anchors.bottom: rootItem.bottom
        anchors.left: rootItem.left
        anchors.right: rootItem.right
        color: "red"
        property var _noteColumns: []
        function createColumns() {
            _noteColumns = [];
            const columnCount = editorService.columnCount(_index);
            const noteColumnWidth = noteColumnContainer.width / columnCount;
            const noteColumnHeight = noteColumnContainer.height;
            for (let col = 0; col < columnCount; col++) {
                console.log(`Creating column ${col} for track ${_index}`);
                const noteColumn = noteColumnComponent.createObject(noteColumnContainer);
                console.log(`Column width: ${noteColumnWidth}, height: ${noteColumnHeight}`);
                noteColumn.width = noteColumnWidth;
                noteColumn.height = noteColumnHeight;
                noteColumn.setIndex(col);
                noteColumn.setTrackIndex(_index);
                noteColumn.updateData();
                _noteColumns.push(noteColumn);
            }
        }
        function resize(width, height) {
            noteColumnContainer.width = width;
            noteColumnContainer.height = height;
            const columnCount = editorService.columnCount(_index);
            const noteColumnWidth = width / columnCount;
            const noteColumnHeight = height;
            _noteColumns.forEach(noteColumn => noteColumn.resize(noteColumnWidth, noteColumnHeight));
        }
        Component {
            id: noteColumnComponent
            NoteColumn {
            }
        }
    }
}
