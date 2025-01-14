import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

MenuItem {
    id: rootItem
    contentItem: Item {
        anchors.centerIn: parent
        Text {
            text: rootItem.text
            anchors.left: parent.left
            color: "white"
        }
        Text {
            text: rootItem.action.shortcut || ""
            anchors.right: parent.right
            color: "white"
        }
    }
}
