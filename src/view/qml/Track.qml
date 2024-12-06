import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

Rectangle {
    id: rootItem
    color: index % 2 === 0 ? "lightgray" : "darkgray"
    border.color: "black"
    border.width: 1

    property int index: 0

    Text {
        anchors.centerIn: parent
        text: "Track " + (parent.index + 1)
        color: "black"
    }
}
