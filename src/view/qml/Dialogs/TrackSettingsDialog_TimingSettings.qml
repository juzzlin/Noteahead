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
                onValueChanged: trackSettingsModel.delay = value
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
            Label {
                text: qsTr("Auto note-off offset (ms):")
                Layout.column: 2
                Layout.columnSpan: 2
                Layout.row: 4
                Layout.fillWidth: true
                enabled: autoNoteOffOffsetCheckbox.checked
            }
            SpinBox {
                id: autoNoteOffOffsetSpinBox
                from: 0
                to: 500
                stepSize: 5
                Layout.column: 4
                Layout.columnSpan: 5
                Layout.row: 4
                Layout.fillWidth: true
                editable: true
                enabled: autoNoteOffOffsetCheckbox.checked
                Keys.onReturnPressed: focus = false
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set offset for auto note-off events in milliseconds. This defines the time between a note-off and the following note-on in the same column.")
                onValueChanged: trackSettingsModel.autoNoteOffOffset = value
            }
        }
    }
}
