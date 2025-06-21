import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."
import "../Components"

Dialog {
    id: rootItem
    title: "<strong>" + qsTr("Settings for track") + ` '${editorService.trackName(trackSettingsModel.trackIndex)}'` + "</strong>"
    modal: true
    property var _midiCcSelectors: []
    property bool _initializing: false
    readonly property string _tag: "TrackSettingsDialog"
    function setTrackIndex(trackIndex) {
        trackSettingsModel.trackIndex = trackIndex;
        trackSettingsModel.requestInstrumentData();
    }
    function initialize() {
        uiLogger.info(_tag, "Initializing");
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
        enablePanCheckbox.checked = trackSettingsModel.panEnabled;
        panSpinBox.value = trackSettingsModel.pan;
        enableVolumeCheckbox.checked = trackSettingsModel.volumeEnabled;
        volumeSpinBox.value = trackSettingsModel.volume;
        sendMidiClockCheckbox.checked = trackSettingsModel.sendMidiClock;
        delaySpinBox.value = trackSettingsModel.delay;
        transposeSpinBox.value = trackSettingsModel.transpose;
        initializeMidiCcSelectors();
    }
    function initializeMidiCcSelectors() {
        for (const midiCcSelector of _midiCcSelectors) {
            midiCcSelector.initialize(trackSettingsModel.midiCcEnabled(midiCcSelector.index), trackSettingsModel.midiCcController(midiCcSelector.index), trackSettingsModel.midiCcValue(midiCcSelector.index));
        }
    }
    function saveSettings() {
        trackSettingsModel.applyAll();
        trackSettingsModel.save();
    }
    function _requestApplyAll() {
        trackSettingsModel.applyAll();
    }
    function _requestApplyMidiCc() {
        trackSettingsModel.applyMidiCc();
    }
    function _requestTestSound() {
        if (rootItem.visible) {
            _requestApplyAll();
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
    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                saveSettings();
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
            title: qsTr("MIDI Instrument Settings")
            Layout.fillWidth: true
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
                        onValueChanged: {
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
                        onValueChanged: {
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
                        onValueChanged: {
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
                }
            }
        }
        GroupBox {
            title: qsTr("Pre-defined MIDI CC Settings")
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 8
                width: parent.width
                GridLayout {
                    columns: 9
                    rows: 3
                    width: parent.width
                    CheckBox {
                        id: enableVolumeCheckbox
                        text: qsTr("Set Volume")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 0
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable channel volume for this track")
                        onCheckedChanged: trackSettingsModel.volumeEnabled = checked
                    }
                    SpinBox {
                        id: volumeSpinBox
                        from: 0
                        to: 127
                        enabled: enableVolumeCheckbox.checked
                        Layout.column: 3
                        Layout.row: 0
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial channel volume for this track")
                        onValueChanged: trackSettingsModel.volume = value
                        Keys.onReturnPressed: focus = false
                    }
                    CheckBox {
                        id: enablePanCheckbox
                        text: qsTr("Set Panning")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 1
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable panning for this track")
                        onCheckedChanged: trackSettingsModel.panEnabled = checked
                    }
                    SpinBox {
                        id: panSpinBox
                        from: 0
                        to: 127
                        enabled: enablePanCheckbox.checked
                        Layout.column: 3
                        Layout.row: 1
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set initial panning for this track (64 = center)")
                        onValueChanged: trackSettingsModel.pan = value
                        Keys.onReturnPressed: focus = false
                    }
                    CheckBox {
                        id: enableCutoffCheckbox
                        text: qsTr("Set Cutoff")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 2
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Enable/disable filter cutoff for this track")
                        onCheckedChanged: trackSettingsModel.cutoffEnabled = checked
                    }
                    SpinBox {
                        id: cutoffSpinBox
                        from: 0
                        to: 127
                        enabled: enableCutoffCheckbox.checked
                        Layout.column: 3
                        Layout.row: 2
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
        GroupBox {
            title: qsTr("Generic MIDI CC Settings")
            Layout.fillWidth: true
            width: parent.width
            ColumnLayout {
                spacing: 8
                width: parent.width
                Repeater {
                    id: midiCcRepeater
                    model: trackSettingsModel.midiCcSlots
                    MidiCcSelector {
                    }
                    onItemAdded: (index, item) => {
                        item.index = index;
                        _midiCcSelectors.push(item);
                        item.settingsChanged.connect(() => {
                                trackSettingsModel.setMidiCcEnabled(item.index, item.enabled());
                                trackSettingsModel.setMidiCcController(item.index, item.controller());
                                trackSettingsModel.setMidiCcValue(item.index, item.value());
                                rootItem._requestApplyAll();
                            });
                    }
                }
            }
        }
        GroupBox {
            title: qsTr("Miscellanous MIDI Settings")
            Layout.fillWidth: true
            width: parent.width
            ColumnLayout {
                spacing: 8
                width: parent.width
                GridLayout {
                    columns: 9
                    rows: 2
                    width: parent.width
                    CheckBox {
                        id: sendMidiClockCheckbox
                        text: qsTr("Send MIDI clock")
                        Layout.column: 0
                        Layout.columnSpan: 2
                        Layout.row: 0
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Send MIDI clock for this track")
                        onCheckedChanged: trackSettingsModel.sendMidiClock = checked
                    }
                    CheckBox {
                        id: sendTransport
                        text: qsTr("Send transport")
                        Layout.column: 2
                        Layout.columnSpan: 2
                        Layout.row: 0
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Send transport events (Start/Stop)")
                        onCheckedChanged: trackSettingsModel.sendTransport = checked
                    }
                    Label {
                        text: qsTr("Delay (ms):")
                        Layout.column: 5
                        Layout.row: 0
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: delaySpinBox
                        from: -999
                        to: 999
                        editable: true
                        Layout.column: 6
                        Layout.row: 0
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set delay for MIDI messages on this channel in milliseconds")
                        onValueChanged: {
                            trackSettingsModel.delay = value;
                        }
                        Keys.onReturnPressed: focus = false
                    }
                    LayoutSeparator {
                        Layout.row: 1
                    }
                    Label {
                        text: qsTr("Transpose (semitones):")
                        Layout.column: 5
                        Layout.row: 2
                        Layout.fillWidth: true
                    }
                    SpinBox {
                        id: transposeSpinBox
                        from: -24
                        to: 24
                        editable: true
                        Layout.column: 6
                        Layout.row: 2
                        Layout.fillWidth: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Set transposition for MIDI notes on this channel in semitones")
                        onValueChanged: {
                            trackSettingsModel.transpose = value;
                        }
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
