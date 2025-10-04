import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

GroupBox {
    title: qsTr("Audio")
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        GridLayout {
            columns: 1
            rows: 2
            width: parent.width
            CheckBox {
                id: enableCutoffCheckbox
                text: qsTr("Enable audio recording from default source when playing.\nAudio files will appear next to the current project file.")
                checked: settingsService.recordingEnabled()
                Layout.row: 0
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Enable/disable audio recording")
                onCheckedChanged: settingsService.setRecordingEnabled(checked)
            }
        }
    }
}
