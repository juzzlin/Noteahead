import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("Miscellanous MIDI Settings")
    Layout.fillWidth: true
    width: parent.width
    function initialize(): void {
        sendMidiClockCheckbox.checked = trackSettingsModel.sendMidiClock;
        delaySpinBox.value = trackSettingsModel.delay;
        transposeSpinBox.value = trackSettingsModel.transpose;
        velocityJitterSpinBox.value = trackSettingsModel.velocityJitter;
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
                text: qsTr("Velocity jitter (%):")
                Layout.column: 0
                Layout.row: 2
                Layout.fillWidth: true
            }
            SpinBox {
                id: velocityJitterSpinBox
                from: 0
                to: 100
                editable: true
                Layout.column: 2
                Layout.row: 2
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set jitter on MIDI velocity to simulate e.g. a more natural piano")
                onValueChanged: {
                    trackSettingsModel.velocityJitter = value;
                }
                Keys.onReturnPressed: focus = false
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
