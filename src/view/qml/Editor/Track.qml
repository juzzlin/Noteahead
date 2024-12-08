import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15

Rectangle {
    id: rootItem
    color: _index % 2 === 0 ? "lightgray" : "darkgray"
    border.color: "black"
    border.width: 1

    property int _index: 0

    property string _name

    function index() {
        return _index;
    }

    function setIndex(index) {
        _index = index;
    }

    function setName(name) {
        _name = name;
    }

    signal nameChanged(string name)

    Rectangle {
        id: trackHeader
        anchors.top: parent.top
        height: 32
        width: parent.width
        TextField {
            text: _name
            placeholderText: qsTr("Track name")
            font.pixelSize: 24
            height: parent.height
            width: parent.width
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            padding: 0  // Remove default padding
            onTextChanged: rootItem.nameChanged(text)
        }
    }
}
