import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../ToolBar"

Item {
    id: rootItem
    signal invertedMuteRequested
    signal invertedSoloRequested
    signal muteRequested
    signal soloRequested
    signal unmuteRequested
    signal unsoloRequested
    function setMuted(mute) {
        muteButton.setToggled(mute);
    }
    function setSoloed(solo) {
        soloButton.setToggled(solo);
    }
    ToolBarButtonBase {
        id: muteButton
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        onLeftClicked: {
            if (isToggled()) {
                rootItem.unmuteRequested();
            } else {
                rootItem.muteRequested();
            }
            focus = false;
        }
        onRightClicked: {
            rootItem.invertedMuteRequested();
            focus = false;
        }
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space) {
                event.accepted = true;
            }
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Mute")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/mute.png");
            setToggleColor("#ff0000");
        }
    }
    ToolBarButtonBase {
        id: soloButton
        anchors.top: muteButton.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        onLeftClicked: {
            if (isToggled()) {
                rootItem.unsoloRequested();
            } else {
                rootItem.soloRequested();
            }
            focus = false;
        }
        onRightClicked: {
            rootItem.invertedSoloRequested();
            focus = false;
        }
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space) {
                event.accepted = true;
            }
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Solo")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/solo.png");
            setToggleColor("#00ff00");
        }
    }
}
