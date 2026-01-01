import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

GroupBox {
    title: qsTr("Standard MIDI CC Settings")
    Layout.fillWidth: true
    function initialize(): void {
        enableCutoffCheckbox.checked = trackSettingsModel.cutoffEnabled;
        cutoffSpinBox.value = trackSettingsModel.cutoff;
        enablePanCheckbox.checked = trackSettingsModel.panEnabled;
        panSpinBox.value = trackSettingsModel.pan;
        enableVolumeCheckbox.checked = trackSettingsModel.volumeEnabled;
        volumeSpinBox.value = trackSettingsModel.volume;
    }
    ColumnLayout {
        spacing: 8
        width: parent.width
        GridLayout {
            columns: 9
            rows: 3
            width: parent.width
            CheckBox {
                id: enableVolumeCheckbox
                text: qsTr("Set Volume (CC #7)")
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
                onValueModified: trackSettingsModel.volume = value
                Keys.onReturnPressed: focus = false
            }
            CheckBox {
                id: enablePanCheckbox
                text: qsTr("Set Panning (CC #10)")
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
                onValueModified: trackSettingsModel.pan = value
                Keys.onReturnPressed: focus = false
            }
            CheckBox {
                id: enableCutoffCheckbox
                text: qsTr("Set Cutoff (CC #74)")
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
                onValueModified: trackSettingsModel.cutoff = value
                Keys.onReturnPressed: focus = false
            }
        }
    }
}
