import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("Audio")
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        GridLayout {
            width: parent.width
            CheckBox {
                id: enableAudioRecordingCheckbox
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
            LayoutSeparator {
                Layout.row: 1
            }
            Label {
                text: qsTr("Buffer size (samples):")
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.row: 2
                Layout.fillWidth: true
            }
            SpinBox {
                id: audioBufferSizeSpinBox
                from: 32
                to: 4096
                stepSize: 32
                enabled: enableAudioRecordingCheckbox.checked
                value: settingsService.audioBufferSize()
                Layout.column: 3
                Layout.row: 2
                Layout.fillWidth: true
                editable: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set buffer size for audio recording")
                onValueChanged: settingsService.setAudioBufferSize(value)
                Keys.onReturnPressed: focus = false
            }
        }
    }
}
