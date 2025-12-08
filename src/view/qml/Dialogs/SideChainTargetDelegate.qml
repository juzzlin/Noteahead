import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    property int targetIndex
    title: qsTr("Target %1").arg(targetIndex)
    function initialize(enabled: bool, controller: int, targetValue: int, releaseValue: int): void {
        targetEnabledCheckBox.checked = enabled;
        controllerSpinBox.value = controller;
        targetValueSpinBox.value = targetValue;
        releaseValueSpinBox.value = releaseValue;
    }
    GridLayout {
        width: parent.width
        Layout.fillWidth: true
        CheckBox {
            id: targetEnabledCheckBox
            text: qsTr("Enabled")
            onCheckedChanged: trackSettingsModel.setSideChainTargetEnabled(targetIndex, checked)
            Layout.row: 0
            Layout.column: 0
        }
        Label {
            text: qsTr("Controller:")
            Layout.row: 1
            Layout.column: 0
        }
        SpinBox {
            id: controllerSpinBox
            from: 0
            to: 127
            editable: true
            enabled: targetEnabledCheckBox.checked
            onValueChanged: trackSettingsModel.setSideChainTargetController(targetIndex, value)
            Keys.onReturnPressed: focus = false
            Layout.fillWidth: true
            Layout.row: 1
            Layout.column: 1
        }
        Label {
            text: qsTr("Target Value:")
            Layout.row: 1
            Layout.column: 2
        }
        SpinBox {
            id: targetValueSpinBox
            from: 0
            to: 127
            editable: true
            enabled: targetEnabledCheckBox.checked
            onValueChanged: trackSettingsModel.setSideChainTargetTargetValue(targetIndex, value)
            Keys.onReturnPressed: focus = false
            Layout.fillWidth: true
            Layout.row: 1
            Layout.column: 3
        }
        Label {
            text: qsTr("Release Value:")
            Layout.row: 1
            Layout.column: 4
        }
        SpinBox {
            id: releaseValueSpinBox
            from: 0
            to: 127
            editable: true
            enabled: targetEnabledCheckBox.checked
            onValueChanged: trackSettingsModel.setSideChainTargetReleaseValue(targetIndex, value)
            Keys.onReturnPressed: focus = false
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.row: 1
            Layout.column: 5
        }
    }
}
