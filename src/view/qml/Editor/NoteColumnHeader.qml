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
    signal muteRequested
    signal muteAllRequested
    signal nameChanged(string name)
    signal soloRequested
    signal unmuteRequested
    signal unmuteAllRequested
    signal unsoloRequested
    signal velocityScaleRequested
    property bool _focused: false
    property int _index: 0
    function setFocused(focused) {
        _focused = focused;
    }
    function setIndex(index) {
        _index = index;
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
    function setVelocityScale(value) {
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
        }
        TextField {
            id: nameField
            color: _focused ? "black" : Constants.trackHeaderTextColor(_index)
            background: Rectangle {
                color: _focused ? Constants.trackHeaderTextColor(_index) : "transparent"
                radius: 12
            }
            font.bold: _focused
            font.pixelSize: Constants.trackHeaderFontSize
            font.family: "monospace"
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
        muteSoloButtons.muteAllRequested.connect(rootItem.muteAllRequested);
        muteSoloButtons.muteRequested.connect(rootItem.muteRequested);
        muteSoloButtons.soloRequested.connect(rootItem.soloRequested);
        muteSoloButtons.unmuteAllRequested.connect(rootItem.unmuteAllRequested);
        muteSoloButtons.unmuteRequested.connect(rootItem.unmuteRequested);
        muteSoloButtons.unsoloRequested.connect(rootItem.unsoloRequested);
    }
}
