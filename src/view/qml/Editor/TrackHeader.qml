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
    signal muteRequested
    signal nameChanged(string name)
    signal newColumnRequested
    signal soloRequested
    signal trackSettingsDialogRequested
    signal unmuteRequested
    signal unsoloRequested
    property bool _focused: false
    function setFocused(focused) {
        _focused = focused;
    }
    function setName(name) {
        nameField.text = name;
    }
    function setMuted(mute) {
        muteSoloButtons.setMuted(mute);
    }
    function setSoloed(solo) {
        muteSoloButtons.setSoloed(solo);
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
            width: parent.width - trackSettingsButton.width - trackHeaderColumnButtons.width - muteSoloButtons.width
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
        TrackHeaderColumnButtons {
            id: trackHeaderColumnButtons
            height: parent.height
            width: height / 2 + 10
        }
        MuteSoloButtons {
            id: muteSoloButtons
            height: parent.height
            width: height / 2 + 10
        }
    }
    Component.onCompleted: {
        trackHeaderColumnButtons.columnDeletionRequested.connect(rootItem.columnDeletionRequested);
        trackHeaderColumnButtons.newColumnRequested.connect(rootItem.newColumnRequested);
        muteSoloButtons.muteRequested.connect(rootItem.muteRequested);
        muteSoloButtons.soloRequested.connect(rootItem.soloRequested);
        muteSoloButtons.unmuteRequested.connect(rootItem.unmuteRequested);
        muteSoloButtons.unsoloRequested.connect(rootItem.unsoloRequested);
    }
}
