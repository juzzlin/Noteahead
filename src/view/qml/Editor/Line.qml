import QtQuick 2.15
import ".."

Rectangle {
    id: rootItem
    color: Constants.noteColumnCellBackgroundColor
    border.color: Constants.noteColumnCellBorderColor
    border.width: 1
    property string note: ""
    property int index: 0
    Text {
        id: text
        anchors.centerIn: parent
        text: rootItem.note
        font.pixelSize: 14
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
}
