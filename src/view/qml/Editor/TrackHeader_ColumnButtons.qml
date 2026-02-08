import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../ToolBar"

Item {
    id: rootItem
    signal columnDeletionRequested
    signal newColumnRequested
    ToolBarButtonBase {
        id: addColumnButton
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        enabled: !UiService.isPlaying()
        onClicked: {
            rootItem.newColumnRequested();
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
        ToolTip.text: qsTr("Add a new note column")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/add_box.png");
        }
    }
    ToolBarButtonBase {
        id: removeColumnButton
        anchors.top: addColumnButton.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        enabled: !UiService.isPlaying()
        onClicked: {
            rootItem.columnDeletionRequested();
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
        ToolTip.text: qsTr("Remove the last note column")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/del_box.png");
        }
    }
}
