import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("MIDI Effects")
    Layout.fillWidth: true
    width: parent.width
    function initialize(): void {
        velocityJitterSpinBox.value = trackSettingsModel.velocityJitter;
    }
    ColumnLayout {
        spacing: 8
        width: parent.width
        GridLayout {
            columns: 9
            rows: 1
            width: parent.width
            Label {
                text: qsTr("Random velocity jitter (%):")
                Layout.column: 0
                Layout.row: 0
                Layout.fillWidth: true
            }
            SpinBox {
                id: velocityJitterSpinBox
                from: 0
                to: 100
                editable: true
                Layout.column: 2
                Layout.row: 0
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
        }
    }
}
