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
        enablePatchCheckbox.checked = trackSettingsModel.patchEnabled;
        if (trackSettingsModel.patchEnabled) {
            patchSpinBox.value = trackSettingsModel.patch;
        }
        enableBankCheckbox.checked = trackSettingsModel.bankEnabled;
        if (trackSettingsModel.bankEnabled) {
            bankLsbSpinBox.value = trackSettingsModel.bankLsb;
            bankMsbSpinBox.value = trackSettingsModel.bankMsb;
            swapBankByteOrderCheckBox.checked = trackSettingsModel.bankByteOrderSwapped;
        }
    }
    function saveSettings() {
        trackSettingsModel.portName = portNameDropdown.currentText;
        trackSettingsModel.channel = channelDropdown.currentValue - 1;
        trackSettingsModel.patchEnabled = enablePatchCheckbox.checked;
        if (trackSettingsModel.patchEnabled) {
            trackSettingsModel.patch = patchSpinBox.value;
        }
        trackSettingsModel.bankEnabled = enableBankCheckbox.checked;
        if (trackSettingsModel.bankEnabled) {
            trackSettingsModel.bankLsb = bankLsbSpinBox.value;
            trackSettingsModel.bankMsb = bankMsbSpinBox.value;
            trackSettingsModel.bankByteOrderSwapped = swapBankByteOrderCheckBox.checked;
        }
        trackSettingsModel.applySettings();
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                saveSettings();
                trackSettingsModel.save();
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
                        model: midiService.availableMidiPorts
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
                        onCurrentTextChanged: trackSettingsModel.applySettings()
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
                        onCurrentValueChanged: trackSettingsModel.applySettings()
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
                        onValueChanged: trackSettingsModel.applySettings()
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial patch for this track. Note that some synths will add 1 to the chosen value so that 0 means 1.")
                    }
                    CheckBox {
                        id: enableBankCheckbox
                        text: qsTr("Enable Bank")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 3
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable bank setting for this track")
                    }
                    Label {
                        text: qsTr("Bank (LSB):")
                        Layout.column: 2
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: bankLsbSpinBox
                        from: 0
                        to: 127
                        enabled: enableBankCheckbox.checked
                        Layout.column: 3
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                        onValueChanged: trackSettingsModel.applySettings()
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial bank for this track (LSB)")
                    }
                    Label {
                        text: qsTr("Bank (MSB):")
                        Layout.column: 5
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: bankMsbSpinBox
                        from: 0
                        to: 127
                        enabled: enableBankCheckbox.checked
                        Layout.column: 6
                        Layout.columnSpan: 1
                        Layout.row: 3
                        Layout.fillWidth: true
                        onValueChanged: trackSettingsModel.applySettings()
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial bank for this track (MSB)")
                    }
                    CheckBox {
                        id: swapBankByteOrderCheckBox
                        text: qsTr("Swap LSB/MSB")
                        enabled: enableBankCheckbox.checked
                        Layout.column: 7
                        Layout.columnSpan: 2
                        Layout.row: 3
                        Layout.fillWidth: true
                        onCheckedChanged: trackSettingsModel.applySettings()
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Swap the send order of LSB and MSB bytes")
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
