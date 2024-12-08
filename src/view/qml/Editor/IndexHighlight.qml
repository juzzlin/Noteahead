import QtQuick 2.15

Item {
    property int index: 0
    Rectangle {
        id: highlight16
        visible: !(index % 16)
        anchors.fill: parent
        color: "white"
        opacity: 0.2
    }
    Rectangle {
        id: highlight8
        visible: !(index % 4) && !(index % 8) && (index % 16)
        anchors.fill: parent
        color: "white"
        opacity: 0.1
    }
    Rectangle {
        id: highlight4
        visible: !(index % 4) && (index % 8) && (index % 16)
        anchors.fill: parent
        color: "white"
        opacity: 0.05
    }
}
