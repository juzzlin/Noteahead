import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Rectangle {
    id: rootItem
    height: Constants.trackHeaderHeight
    width: parent.width
    color: Constants.trackHeaderBackgroundColor
    border.color: Constants.trackHeaderBorderColor
    border.width: 1
    signal nameChanged(string name)
    signal newColumnRequested
    property bool _focused: false
    function setFocused(focused) {
        _focused = focused;
    }
    Row {
        height: parent.height
        width: parent.width
        TextField {
            id: nameField
            text: _name
            placeholderText: qsTr("Track name")
            color: _focused ? "black" : Constants.trackHeaderTextColor
            background: Rectangle {
                color: _focused ? Constants.trackHeaderTextColor : "transparent"
            }
            font.bold: _focused
            font.pixelSize: Constants.trackHeaderFontSize
            font.family: "monospace"
            height: parent.height
            width: parent.width - newColumnButton.width
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            padding: 0  // Remove default padding
            onTextChanged: rootItem.nameChanged(text)
        }
        Button {
            id: newColumnButton
            height: parent.height
            width: height
            text: "+"
            font.bold: true
            font.pixelSize: Constants.trackHeaderFontSize
            font.family: "monospace"
            onClicked: rootItem.newColumnRequested()
        }
    }
}
