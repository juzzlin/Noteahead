import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings for track") + ` '${editorService.trackName(trackSettingsModel.trackIndex)}'` + "</strong>"
    modal: true
    function setTrackIndex(trackIndex) {
        trackSettingsModel.trackIndex = trackIndex;
        trackSettingsModel.requestInstrumentData();
    }
    function initialize() {
        portNameDropdown.setSelected(trackSettingsModel.portName);
        channelDropdown.setSelected(trackSettingsModel.channel + 1);
        enableBankCheckbox.checked = trackSettingsModel.bankEnabled;
        bankLsbSpinBox.value = trackSettingsModel.bankLsb;
        bankMsbSpinBox.value = trackSettingsModel.bankMsb;
        swapBankByteOrderCheckBox.checked = trackSettingsModel.bankByteOrderSwapped;
        enableCutoffCheckbox.checked = trackSettingsModel.cutoffEnabled;
        cutoffSpinBox.value = trackSettingsModel.cutoff;
        enablePatchCheckbox.checked = trackSettingsModel.patchEnabled;
        patchSpinBox.value = trackSettingsModel.patch;
        enableVolumeCheckbox.checked = trackSettingsModel.volumeEnabled;
        volumeSpinBox.value = trackSettingsModel.volume;
    }
    function saveSettings() {
        trackSettingsModel.applyAll();
        trackSettingsModel.save();
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                saveSettings();
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
        Button {
            text: qsTr("Test")
            DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Test current settings by triggering the middle C")
            onClicked: trackSettingsModel.requestTestSound(UiService._activeVelocity)
        }
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        GroupBox {
            title: qsTr("MIDI Settings")
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 8
                width: parent.width
                GridLayout {
                    columns: 9
                    rows: 2
                    width: parent.width
                    Label {
                        text: qsTr("Port:")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 0
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        id: portNameDropdown
                        model: trackSettingsModel.availableMidiPorts
                        currentIndex: 0
                        Layout.column: 2
                        Layout.columnSpan: 7
                        Layout.row: 0
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set MIDI port for this track")
                        function setSelected(text) {
                            currentIndex = find(text);
                        }
                        onCurrentTextChanged: trackSettingsModel.portName = currentText
                    }
                    Label {
                        text: qsTr("Channel:")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 1
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        id: channelDropdown
                        model: ListModel {
                            // Populate channels 1–16
                            Component.onCompleted: {
                                for (let i = 1; i <= 16; i++)
                                    append({
                                            "channel": i
                                        });
                            }
                        }
                        textRole: "channel"
                        currentIndex: 0
                        Layout.column: 2
                        Layout.columnSpan: 7
                        Layout.row: 1
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set MIDI channel (1-16)")
                        function setSelected(text) {
                            currentIndex = find(text);
                        }
                        onCurrentValueChanged: trackSettingsModel.channel = currentValue - 1
                    }
                    Label {
                        text: qsTr("Patch:")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 2
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        id: enablePatchCheckbox
                        text: qsTr("Enable Patch")
                        Layout.column: 2
                        Layout.columnSpan: 2
                        Layout.row: 2
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable patch setting for this track")
                        onCheckedChanged: trackSettingsModel.patchEnabled = checked
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
                        onValueChanged: trackSettingsModel.patch = value
                        Keys.onReturnPressed: focus = false
                    }
                    Label {
                        text: qsTr("Bank:")
                        Layout.column: 0
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        id: enableBankCheckbox
                        text: qsTr("Enable Bank")
                        Layout.column: 2
                        Layout.columnSpan: 2
                        Layout.row: 3
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable bank setting for this track")
                        onCheckedChanged: trackSettingsModel.bankEnabled = checked
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
                        onValueChanged: trackSettingsModel.bankMsb = value
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
                        onValueChanged: trackSettingsModel.bankLsb = value
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
                        onCheckedChanged: trackSettingsModel.bankByteOrderSwapped = checked
                    }
                    Label {
                        text: qsTr("Volume:")
                        Layout.column: 0
                        Layout.row: 4
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        id: enableVolumeCheckbox
                        text: qsTr("Enable Volume")
                        Layout.column: 2
                        Layout.columnSpan: 2
                        Layout.row: 4
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable channel volume for this track")
                        onCheckedChanged: trackSettingsModel.volumeEnabled = checked
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
                        onValueChanged: trackSettingsModel.volume = value
                        Keys.onReturnPressed: focus = false
                    }
                    Label {
                        text: qsTr("Cutoff:")
                        Layout.column: 0
                        Layout.row: 5
                        Layout.fillWidth: true
                    }
                    CheckBox {
                        id: enableCutoffCheckbox
                        text: qsTr("Enable Cutoff")
                        Layout.column: 2
                        Layout.columnSpan: 2
                        Layout.row: 5
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable filter cutoff for this track")
                        onCheckedChanged: trackSettingsModel.cutoffEnabled = checked
                    }
                    Label {
                        text: qsTr("MSB:")
                        Layout.column: 4
                        Layout.row: 5
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: cutoffSpinBox
                        from: 0
                        to: 127
                        enabled: enableCutoffCheckbox.checked
                        Layout.column: 5
                        Layout.columnSpan: 1
                        Layout.row: 5
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial filter cutoff for this track")
                        onValueChanged: trackSettingsModel.cutoff = value
                        Keys.onReturnPressed: focus = false
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        visible = false;
        trackSettingsModel.instrumentDataReceived.connect(initialize);
    }
}
