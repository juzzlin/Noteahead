import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("MIDI Events for Line") + ` ${editorService.position.line} on Track ${editorService.position.track + 1}` + "</strong>"
    modal: true
    function initialize() {
        enableBankCheckbox.checked = eventSelectionModel.bankEnabled;
        bankLsbSpinBox.value = eventSelectionModel.bankLsb;
        bankMsbSpinBox.value = eventSelectionModel.bankMsb;
        swapBankByteOrderCheckBox.checked = eventSelectionModel.bankByteOrderSwapped;
        enableCutoffCheckbox.checked = eventSelectionModel.cutoffEnabled;
        cutoffSpinBox.value = eventSelectionModel.cutoff;
        enablePatchCheckbox.checked = eventSelectionModel.patchEnabled;
        patchSpinBox.value = eventSelectionModel.patch;
        enablePanCheckbox.checked = eventSelectionModel.panEnabled;
        panSpinBox.value = eventSelectionModel.pan;
        enableVolumeCheckbox.checked = eventSelectionModel.volumeEnabled;
        volumeSpinBox.value = eventSelectionModel.volume;
    }
    function requestData() {
        eventSelectionModel.requestData();
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                eventSelectionModel.save();
                rootItem.accepted();
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Save current settings")
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: rootItem.rejected()
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Don't save current settings")
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        GroupBox {
            title: qsTr("Predefined MIDI events")
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 8
                width: parent.width
                GridLayout {
                    columns: 9
                    rows: 2
                    width: parent.width
                    CheckBox {
                        id: enablePatchCheckbox
                        text: qsTr("Set Patch")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 2
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable patch setting for this track")
                        onCheckedChanged: eventSelectionModel.patchEnabled = checked
                    }
                    SpinBox {
                        id: patchSpinBox
                        from: 0
                        to: 127
                        enabled: enablePatchCheckbox.checked
                        Layout.column: 4
                        Layout.columnSpan: 5
                        Layout.row: 2
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial patch for this track. Note that some synths will add 1 to the chosen value so that 0 means 1.")
                        onValueChanged: eventSelectionModel.patch = value
                        Keys.onReturnPressed: focus = false
                    }
                    CheckBox {
                        id: enableBankCheckbox
                        text: qsTr("Set Bank")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 3
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable bank setting for this track")
                        onCheckedChanged: eventSelectionModel.bankEnabled = checked
                    }
                    Label {
                        text: qsTr("MSB/LSB:")
                        Layout.column: 4
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: bankMsbSpinBox
                        from: 0
                        to: 127
                        enabled: enableBankCheckbox.checked
                        Layout.column: 5
                        Layout.row: 3
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial bank for this track (MSB)")
                        onValueChanged: eventSelectionModel.bankMsb = value
                        Keys.onReturnPressed: focus = false
                    }
                    SpinBox {
                        id: bankLsbSpinBox
                        from: 0
                        to: 127
                        enabled: enableBankCheckbox.checked
                        Layout.column: 6
                        Layout.row: 3
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial bank for this track (LSB)")
                        onValueChanged: eventSelectionModel.bankLsb = value
                        Keys.onReturnPressed: focus = false
                    }
                    CheckBox {
                        id: swapBankByteOrderCheckBox
                        text: qsTr("Swap MSB/LSB")
                        enabled: enableBankCheckbox.checked
                        Layout.column: 7
                        Layout.columnSpan: 2
                        Layout.row: 3
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Swap the send order of LSB and MSB bytes")
                        onCheckedChanged: eventSelectionModel.bankByteOrderSwapped = checked
                    }
                    CheckBox {
                        id: enableVolumeCheckbox
                        text: qsTr("Set Volume")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 4
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable channel volume for this track")
                        onCheckedChanged: eventSelectionModel.volumeEnabled = checked
                    }
                    Label {
                        text: qsTr("MSB:")
                        Layout.column: 4
                        Layout.row: 4
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: volumeSpinBox
                        from: 0
                        to: 127
                        enabled: enableVolumeCheckbox.checked
                        Layout.column: 5
                        Layout.columnSpan: 1
                        Layout.row: 4
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial channel volume for this track")
                        onValueChanged: eventSelectionModel.volume = value
                        Keys.onReturnPressed: focus = false
                    }
                    CheckBox {
                        id: enablePanCheckbox
                        text: qsTr("Set Panning")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 5
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable panning for this track")
                        onCheckedChanged: eventSelectionModel.panEnabled = checked
                    }
                    Label {
                        text: qsTr("MSB:")
                        Layout.column: 4
                        Layout.row: 5
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: panSpinBox
                        from: 0
                        to: 127
                        enabled: enablePanCheckbox.checked
                        Layout.column: 5
                        Layout.columnSpan: 1
                        Layout.row: 5
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial panning for this track (64 = center)")
                        onValueChanged: eventSelectionModel.pan = value
                        Keys.onReturnPressed: focus = false
                    }
                    CheckBox {
                        id: enableCutoffCheckbox
                        text: qsTr("Set Cutoff")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 6
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable filter cutoff for this track")
                        onCheckedChanged: eventSelectionModel.cutoffEnabled = checked
                    }
                    Label {
                        text: qsTr("MSB:")
                        Layout.column: 4
                        Layout.row: 6
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: cutoffSpinBox
                        from: 0
                        to: 127
                        enabled: enableCutoffCheckbox.checked
                        Layout.column: 5
                        Layout.columnSpan: 1
                        Layout.row: 6
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial filter cutoff for this track")
                        onValueChanged: eventSelectionModel.cutoff = value
                        Keys.onReturnPressed: focus = false
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        visible = false;
        eventSelectionModel.dataReceived.connect(rootItem.initialize);
    }
}
