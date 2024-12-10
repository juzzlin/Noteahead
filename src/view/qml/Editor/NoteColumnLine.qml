import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnCellBackgroundColor
    border.color: Constants.noteColumnCellBorderColor
    border.width: 1
    property string note: ""
    property int index: 0
    property bool _focused: false
    function setFocused(focused) {
        _focused = focused;
    }
    Text {
        id: text
        anchors.centerIn: parent
        text: rootItem.note
        font.pixelSize: parent.height * 0.8
        font.family: "monospace"
        color: Constants.noteColumnTextColor
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
    }
    IndexHighlight {
        anchors.fill: parent
        index: parent.index
    }
    Rectangle {
        id: cursor
        anchors.fill: parent
        color: "red"
        opacity: 0.5
        visible: rootItem._focused
    }
}
