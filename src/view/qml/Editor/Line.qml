import QtQuick 2.15

Rectangle {
    id: rootItem
    color: index % 2 === 0 ? "white" : "lightgray"
    border.color: "gray"
    border.width: 1
    property string note: ""
    property int index: 0
    Text {
        id: text
        anchors.centerIn: parent
        text: rootItem.note
        font.pixelSize: 14
        color: "black"
    }
    function resize(width, height) {
        rootItem.width = width;
        rootItem.height = height;
    }
}
