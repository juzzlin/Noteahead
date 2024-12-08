import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Rectangle {
    id: rootItem
    height: 32
    width: parent.width
    color: Constants.trackHeaderBackgroundColor
    border.color: Constants.trackHeaderBorderColor
    border.width: 1
    signal nameChanged(string name)
    TextField {
        text: _name
        placeholderText: qsTr("Track name")
        color: Constants.trackHeaderTextColor
        background: Rectangle {
            color: "transparent"
        }
        font.pixelSize: 24
        height: parent.height
        width: parent.width
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        padding: 0  // Remove default padding
        onTextChanged: rootItem.nameChanged(text)
    }
}
