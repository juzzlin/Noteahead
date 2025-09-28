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
    signal columnSettingsDialogRequested
    signal invertedMuteRequested
    signal invertedSoloRequested
    signal muteRequested
    signal nameChanged(string name)
    signal soloRequested
    signal unmuteRequested
    signal unsoloRequested
    signal velocityScaleRequested
    property bool _focused: false
    property int _index: 0
    function setFocused(focused: bool): void {
        _focused = focused;
    }
    function setIndex(index: int): void {
        _index = index;
    }
    function setName(name: string): void {
        nameField.text = name;
    }
    function setMuted(mute: bool): void {
        muteSoloButtons.setMuted(mute);
    }
    function setSoloed(solo: bool): void {
        muteSoloButtons.setSoloed(solo);
    }
    function setVelocityScale(value: int): void {
        velocityScaleWidget.value = value;
    }
    Row {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        VelocityScale {
            id: velocityScaleWidget
            height: parent.height
            width: parent.height
            onClicked: rootItem.velocityScaleRequested()
            toolTipText: qsTr("Set velocity scale for this COLUMN, 0-100 %. All note velocities will be scaled according to this setting, taking into account the track-level scale.")
        }
        ToolBarButtonBase {
            id: columnSettingsButton
            height: parent.height
            width: height
            enabled: !UiService.isPlaying()
            onClicked: {
                rootItem.columnSettingsDialogRequested();
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
            ToolTip.text: qsTr("Open column settings")
            Component.onCompleted: {
                setScale(0.8);
                setImageSource("../Graphics/settings.svg");
            }
        }
        TextField {
            id: nameField
            color: _focused ? "black" : Constants.trackHeaderTextColor(_index)
            background: Rectangle {
                color: _focused ? Constants.trackHeaderTextColor(_index) : "transparent"
                radius: 12
            }
            font.bold: _focused
            font.pixelSize: settingsService.trackHeaderFontSize
            font.family: "sans"
            height: parent.height
            width: parent.width - muteSoloButtons.width - velocityScaleWidget.width
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
            ToolTip.text: text || qsTr("Set column name")
        }
        MuteSoloButtons {
            id: muteSoloButtons
            height: parent.height
            width: height / 2 + 10
        }
    }
    Component.onCompleted: {
        muteSoloButtons.invertedMuteRequested.connect(rootItem.invertedMuteRequested);
        muteSoloButtons.invertedSoloRequested.connect(rootItem.invertedSoloRequested);
        muteSoloButtons.muteRequested.connect(rootItem.muteRequested);
        muteSoloButtons.soloRequested.connect(rootItem.soloRequested);
        muteSoloButtons.unmuteRequested.connect(rootItem.unmuteRequested);
        muteSoloButtons.unsoloRequested.connect(rootItem.unsoloRequested);
    }
}
