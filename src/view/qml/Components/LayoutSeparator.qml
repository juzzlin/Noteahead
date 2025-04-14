import QtQuick 2.15
import QtQuick.Layouts

Item {
    Layout.column: 0
    Layout.columnSpan: 9
    Layout.fillWidth: true
    height: 11
    Rectangle {
        color: "grey"
        height: 1
        width: parent.width
        anchors.centerIn: parent
    }
}
