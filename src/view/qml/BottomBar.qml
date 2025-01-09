import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Rectangle {
    id: rootItem
    color: "black"
    property string _statusText
    function setStatusText(text) {
        _statusText = text;
        _fadeOutText();
    }
    function _fadeOutText() {
        fadeAnimation.running = false;
        fadeAnimation.running = true;
    }
    Label {
        id: statusText
        text: _statusText
        anchors.centerIn: parent
        color: "white"
        font.pixelSize: height
        opacity: 1
        NumberAnimation {
            id: fadeAnimation
            target: statusText
            property: "opacity"
            from: 1
            to: 0
            duration: 5000
            running: false
        }
    }
    Component.onCompleted: setStatusText(qsTr("Press <b>ESC</b> to edit, <b>SPACE</b> to play"))
}
