import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../ToolBar"

Item {
    id: rootItem
    signal muteTrackRequested
    signal soloTrackRequested
    signal unmuteTrackRequested
    signal unsoloTrackRequested
    function setMuted(mute) {
        muteTrackButton.setToggled(mute);
    }
    function setSoloed(solo) {
        soloTrackButton.setToggled(solo);
    }
    ToolBarButtonBase {
        id: muteTrackButton
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        enabled: !UiService.isPlaying()
        onClicked: {
            if (toggled()) {
                rootItem.unmuteTrackRequested();
            } else {
                rootItem.muteTrackRequested();
            }
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
        ToolTip.text: qsTr("Mute track")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/mute.svg");
            setToggleColor("#ff0000");
        }
    }
    ToolBarButtonBase {
        id: soloTrackButton
        anchors.top: muteTrackButton.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        enabled: !UiService.isPlaying()
        onClicked: {
            if (toggled()) {
                rootItem.unsoloTrackRequested();
            } else {
                rootItem.soloTrackRequested();
            }
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
        ToolTip.text: qsTr("Solo track")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/solo.svg");
            setToggleColor("#00ff00");
        }
    }
}
