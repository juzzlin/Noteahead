import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../ToolBar"

Rectangle {
    id: rootItem
    height: Constants.trackHeaderHeight
    width: parent.width
    color: Constants.trackHeaderBackgroundColor
    border.color: Constants.trackHeaderBorderColor
    border.width: 1
    signal columnDeletionRequested
    signal nameChanged(string name)
    signal newColumnRequested
    signal trackSettingsDialogRequested
    property bool _focused: false
    function setFocused(focused) {
        _focused = focused;
    }
    function setName(name) {
        _name = name;
    }
    Row {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        ToolBarButtonBase {
            id: trackSettingsButton
            height: parent.height
            width: height
            enabled: !UiService.isPlaying()
            onClicked: {
                rootItem.trackSettingsDialogRequested();
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
            ToolTip.text: qsTr("Open track settings")
            Component.onCompleted: {
                setScale(0.8);
                setImageSource("../Graphics/settings.svg");
            }
        }
        TextField {
            id: nameField
            text: _name
            placeholderText: qsTr("Track name")
            color: _focused ? "black" : Constants.trackHeaderTextColor
            background: Rectangle {
                color: _focused ? Constants.trackHeaderTextColor : "transparent"
                radius: 12
            }
            font.bold: _focused
            font.pixelSize: Constants.trackHeaderFontSize
            font.family: "monospace"
            height: parent.height
            width: parent.width - trackSettingsButton.width - columnButtonContainer.width
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            padding: 0  // Remove default padding
            onTextChanged: rootItem.nameChanged(text)
            Keys.onReturnPressed: {
                focus = false;
                UiService.requestFocusOnEditorView();
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Set track name")
        }
        Item {
            id: columnButtonContainer
            height: parent.height
            width: height / 2 + 10
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
                    setImageSource("../Graphics/add_box.svg");
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
                    setImageSource("../Graphics/del_box.svg");
                }
            }
        }
    }
}
