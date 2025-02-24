import QtQuick 2.15

Item {
    id: rootItem
    readonly property int minValue: 0
    readonly property int maxValue: 100
    property int value: 100
    signal clicked
    Rectangle {
        color: "transparent"
        border.color: "white"
        border.width: 1
        height: parent.height * 0.8
        width: 10
        anchors.centerIn: parent
        clip: true
        Rectangle {
            id: bar
            color: Qt.rgba(value / maxValue, 1 - value / maxValue * (1 - 0.647), 0, 1)
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.border.width
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: parent.border.width
            anchors.rightMargin: parent.border.width
            height: (value - minValue) * (parent.height - parent.border.width * 2) / (maxValue - minValue)
        }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: rootItem.clicked()
        cursorShape: Qt.PointingHandCursor
    }
}
