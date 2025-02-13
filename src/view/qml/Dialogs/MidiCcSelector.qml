import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts
import QtQuick.Controls.Universal
import ".."

GridLayout {
    columns: 9
    rows: 1
    property int index
    CheckBox {
        id: enableCcCheckbox
        text: qsTr("Enable MIDI CC #") + index
        Layout.column: 0
        Layout.columnSpan: 2
        Layout.row: 0
        Layout.fillWidth: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Enable/disable filter cutoff for this track")
        onCheckedChanged: trackSettingsModel.cutoffEnabled = checked
    }
    Label {
        text: qsTr("Controller / Value:")
        Layout.column: 2
        Layout.row: 0
        Layout.fillWidth: true
    }
    SpinBox {
        id: midiCc1ControllerSpinBox
        from: 0
        to: 127
        enabled: enableCcCheckbox.checked
        Layout.column: 4
        Layout.row: 0
        Layout.fillWidth: true
        editable: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Set optional MIDI continuous controller value for this track")
        onValueChanged: trackSettingsModel.midiCc1 = value
        Keys.onReturnPressed: focus = false
    }
    SpinBox {
        id: midiCc1SpinBox
        from: 0
        to: 127
        enabled: enableCcCheckbox.checked
        Layout.column: 6
        Layout.row: 0
        Layout.fillWidth: true
        editable: true
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Set optional MIDI continuous controller value for this track")
        onValueChanged: trackSettingsModel.midiCc1 = value
        Keys.onReturnPressed: focus = false
    }
}
