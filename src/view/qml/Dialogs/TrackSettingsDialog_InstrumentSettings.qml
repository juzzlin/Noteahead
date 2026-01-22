import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

ColumnLayout {
    id: rootItem
    spacing: 10
    width: parent.width
    function initialize(): void {
        portNameDropdown.setSelected(trackSettingsModel.portName);
        channelDropdown.setSelected(trackSettingsModel.channel + 1);
        enableBankCheckbox.checked = trackSettingsModel.bankEnabled;
        bankLsbSpinBox.value = trackSettingsModel.bankLsb;
        bankMsbSpinBox.value = trackSettingsModel.bankMsb;
        swapBankByteOrderCheckBox.checked = trackSettingsModel.bankByteOrderSwapped;
        enablePatchCheckbox.checked = trackSettingsModel.patchEnabled;
        patchSpinBox.value = trackSettingsModel.patch;
        transposeSpinBox.value = trackSettingsModel.transpose;
        velocityKeyTrackSpinBox.value = trackSettingsModel.velocityKeyTrack;
        velocityKeyTrackOffsetSpinBox.value = trackSettingsModel.velocityKeyTrackOffset;
    }
    function _requestTestSound(): void {
        if (visible) {
            trackSettingsModel.applyAll();
            testSoundTimer.restart();
        }
    }
    Timer {
        id: testSoundTimer
        interval: 125
        running: false
        repeat: false
        onTriggered: trackSettingsModel.requestTestSound(UiService._activeVelocity)
    }
    GroupBox {
        title: qsTr("Instrument")
        Layout.fillWidth: true
        Layout.fillHeight: true
        ColumnLayout {
            spacing: 8
            width: parent.width
            GridLayout {
                columns: 9
                rows: 3
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
                    onCurrentValueChanged: {
                        trackSettingsModel.channel = currentValue - 1;
                        _requestTestSound();
                    }
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
                    onCheckedChanged: {
                        trackSettingsModel.patchEnabled = checked;
                        if (checked) {
                            _requestTestSound();
                        }
                    }
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
                    onValueModified: {
                        trackSettingsModel.patch = value;
                        _requestTestSound();
                    }
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
                    onCheckedChanged: {
                        trackSettingsModel.bankEnabled = checked;
                        if (checked) {
                            _requestTestSound();
                        }
                    }
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
                    onValueModified: {
                        trackSettingsModel.bankMsb = value;
                        _requestTestSound();
                    }
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
                    onValueModified: {
                        trackSettingsModel.bankLsb = value;
                        _requestTestSound();
                    }
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
                    onCheckedChanged: {
                        trackSettingsModel.bankByteOrderSwapped = checked;
                        _requestTestSound();
                    }
                }
                LayoutSeparator {
                    Layout.row: 4
                }
                Label {
                    text: qsTr("Transpose (semitones):")
                    Layout.column: 0
                    Layout.row: 5
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: transposeSpinBox
                    from: -24
                    to: 24
                    editable: true
                    Layout.column: 2
                    Layout.row: 5
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set transposition for MIDI notes on this channel in semitones")
                    onValueModified: trackSettingsModel.transpose = value
                    Keys.onReturnPressed: focus = false
                }
            }
        }
    }
    GroupBox {
        title: qsTr("Velocity Key Track")
        Layout.fillWidth: true
        GridLayout {
            columns: 5
            width: parent.width
            Label {
                text: qsTr("Amount (%):")
                Layout.fillWidth: true
            }
            SpinBox {
                id: velocityKeyTrackSpinBox
                from: 0
                to: 100
                editable: true
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Linearly reduce velocity for higher notes (0-100%)")
                onValueModified: trackSettingsModel.velocityKeyTrack = value
                Keys.onReturnPressed: focus = false
            }
            Item {
                Layout.preferredWidth: 20
            }
            Label {
                text: qsTr("Offset (notes):")
                Layout.fillWidth: true
            }
            SpinBox {
                id: velocityKeyTrackOffsetSpinBox
                from: 0
                to: 127
                editable: true
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("The note offset where velocity reduction starts")
                onValueModified: trackSettingsModel.velocityKeyTrackOffset = value
                Keys.onReturnPressed: focus = false
            }
        }
    }
    VirtualKeyboard {
        Layout.fillWidth: true
        velocityKeyTrack: trackSettingsModel.velocityKeyTrack
        velocityKeyTrackOffset: trackSettingsModel.velocityKeyTrackOffset
        onNoteOnRequested: note => trackSettingsModel.requestNoteOn(note, UiService._activeVelocity)
        onNoteOffRequested: note => trackSettingsModel.requestNoteOff(note)
    }
}
