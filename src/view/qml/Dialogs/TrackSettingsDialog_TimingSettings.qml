import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("Timing")
    Layout.fillWidth: true
    width: parent.width
    function initialize(): void {
        sendMidiClockCheckbox.checked = trackSettingsModel.sendMidiClock;
        sendTransportCheckbox.checked = trackSettingsModel.sendTransport;
        delaySpinBox.value = trackSettingsModel.delay;
        autoNoteOffOffsetCheckbox.checked = trackSettingsModel.autoNoteOffOffsetEnabled;
        autoNoteOffOffsetSpinBox.value = trackSettingsModel.autoNoteOffOffset;
        autoNoteOffSyncCheckbox.checked = trackSettingsModel.autoNoteOffSyncEnabled;
        autoNoteOffSyncComboBox.currentIndex = autoNoteOffSyncComboBox.indexOfDenominator(trackSettingsModel.autoNoteOffSyncDenominator);
    }
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
                id: sendTransportCheckbox
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
                text: qsTr("Event delay (ms):")
                Layout.column: 5
                Layout.row: 0
                Layout.fillWidth: true
            }
            SpinBox {
                id: delaySpinBox
                from: -Constants.maxEventDelay
                to: Constants.maxEventDelay
                editable: true
                Layout.column: 6
                Layout.row: 0
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set delay for MIDI messages on this channel in milliseconds")
                onValueModified: trackSettingsModel.delay = value
                Keys.onReturnPressed: focus = false
            }
            LayoutSeparator {
                Layout.row: 3
            }
            CheckBox {
                id: autoNoteOffOffsetCheckbox
                text: qsTr("Custom auto note-off")
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.row: 4
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Override the default auto note-off offset")
                onCheckedChanged: trackSettingsModel.autoNoteOffOffsetEnabled = checked
            }
            CheckBox {
                id: autoNoteOffSyncCheckbox
                text: qsTr("Sync to BPM")
                Layout.column: 2
                Layout.columnSpan: 2
                Layout.row: 4
                Layout.fillWidth: true
                enabled: autoNoteOffOffsetCheckbox.checked
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Express the offset as a note length instead of milliseconds, so that it follows the tempo")
                onCheckedChanged: trackSettingsModel.autoNoteOffSyncEnabled = checked
            }
            Label {
                text: autoNoteOffSyncCheckbox.checked ? qsTr("Offset:") : qsTr("Offset (ms):")
                Layout.column: 4
                Layout.row: 4
                Layout.fillWidth: true
                enabled: autoNoteOffOffsetCheckbox.checked
            }
            StackLayout {
                Layout.column: 5
                Layout.columnSpan: 4
                Layout.row: 4
                Layout.fillWidth: true
                currentIndex: autoNoteOffSyncCheckbox.checked ? 1 : 0
                SpinBox {
                    id: autoNoteOffOffsetSpinBox
                    from: 0
                    to: 500
                    stepSize: 5
                    Layout.fillWidth: true
                    editable: true
                    enabled: autoNoteOffOffsetCheckbox.checked
                    Keys.onReturnPressed: focus = false
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set offset for auto note-off events in milliseconds. This defines the time between a note-off and the following note-on in the same column.")
                    onValueModified: trackSettingsModel.autoNoteOffOffset = value
                }
                ComboBox {
                    id: autoNoteOffSyncComboBox
                    Layout.fillWidth: true
                    enabled: autoNoteOffOffsetCheckbox.checked
                    model: songSettingsModel.autoNoteOffSyncDenominators.map(denominator => `1/${denominator}`)
                    function indexOfDenominator(denominator: int): int {
                        const index = songSettingsModel.autoNoteOffSyncDenominators.indexOf(denominator);
                        return index >= 0 ? index : 0;
                    }
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Set offset for auto note-off events as a note length. This defines the time between a note-off and the following note-on in the same column.")
                    onActivated: trackSettingsModel.autoNoteOffSyncDenominator = songSettingsModel.autoNoteOffSyncDenominators[currentIndex]
                }
            }
        }
    }
}
