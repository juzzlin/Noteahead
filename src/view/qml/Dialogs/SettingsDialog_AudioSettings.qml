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
                checked: settingsService.recordingEnabled
                Layout.row: 0
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Enable/disable audio recording")
                onCheckedChanged: {
                    if (settingsService.recordingEnabled !== checked) {
                        settingsService.recordingEnabled = checked
                    }
                }
            }
            CheckBox {
                id: showWaveViewCheckbox
                text: qsTr("Show recording wave view at the bottom of the editor.")
                checked: settingsService.waveViewEnabled
                Layout.row: 1
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Show/hide the recording wave view")
                onCheckedChanged: {
                    if (settingsService.waveViewEnabled !== checked) {
                        settingsService.waveViewEnabled = checked
                    }
                }
            }
            LayoutSeparator {
                Layout.row: 2
            }
            Label {
                text: qsTr("Buffer size (samples):")
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.row: 3
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
                Layout.row: 3
                Layout.fillWidth: true
                editable: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Set buffer size for audio recording")
                onValueChanged: settingsService.setAudioBufferSize(value)
                Keys.onReturnPressed: focus = false
            }
            LayoutSeparator {
                Layout.row: 4
            }
            Label {
                text: qsTr("Input Device:")
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.row: 5
                Layout.fillWidth: true
            }
            ComboBox {
                id: audioDeviceComboBox
                Layout.column: 3
                Layout.row: 5
                Layout.fillWidth: true
                enabled: enableAudioRecordingCheckbox.checked
                model: audioSettingsModel.inputDevices
                textRole: "name"
                valueRole: "id"
                Component.onCompleted: {
                    currentIndex = indexOfValue(audioSettingsModel.selectedInputDeviceId);
                }
                onActivated: {
                    audioSettingsModel.selectedInputDeviceId = currentValue;
                }
                Connections {
                    target: audioSettingsModel
                    function onInputDevicesChanged() {
                        audioDeviceComboBox.currentIndex = audioDeviceComboBox.indexOfValue(audioSettingsModel.selectedInputDeviceId);
                    }
                }
            }
            Button {
                text: qsTr("Refresh")
                Layout.column: 4
                Layout.row: 5
                enabled: enableAudioRecordingCheckbox.checked
                onClicked: audioSettingsModel.refreshInputDevices()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Refresh device list")
            }
        }
    }
}
