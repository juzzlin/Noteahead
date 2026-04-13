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
        enabled: !UiService.isPlaying()
        opacity: enabled ? 1.0 : 0.5
        GridLayout {
            width: parent.width
            CheckBox {
                id: enableAudioRecordingCheckbox
                text: qsTr("Enable audio recording and playback from default source when playing.\nAudio files will appear next to the current project file.")
                checked: settingsService.recordingEnabled
                Layout.row: 0
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Enable/disable audio recording and playback")
                onCheckedChanged: {
                    if (settingsService.recordingEnabled !== checked) {
                        settingsService.recordingEnabled = checked
                    }
                }
            }
            CheckBox {
                id: showWaveViewCheckbox
                text: qsTr("Show recording and playback wave view at the bottom of the editor.")
                checked: settingsService.waveViewEnabled
                Layout.row: 1
                Layout.fillWidth: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Show/hide the wave view")
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
                enabled: enableAudioRecordingCheckbox.checked && !settingsService.jackSyncEnabled
                value: settingsService.audioBufferSize()
                Layout.column: 3
                Layout.row: 3
                Layout.fillWidth: true
                editable: true
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: settingsService.jackSyncEnabled ? qsTr("Buffer size is managed by JACK server") : qsTr("Set buffer size for audio recording")
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
                id: audioInputDeviceComboBox
                Layout.column: 3
                Layout.row: 5
                Layout.fillWidth: true
                enabled: enableAudioRecordingCheckbox.checked && !settingsService.jackSyncEnabled
                model: audioSettingsModel.inputDevices
                textRole: "name"
                valueRole: "id"
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: settingsService.jackSyncEnabled ? qsTr("Device selection is managed by JACK routing") : qsTr("Select audio input device")
                Component.onCompleted: {
                    currentIndex = indexOfValue(audioSettingsModel.selectedInputDeviceId);
                }
                onActivated: {
                    audioSettingsModel.selectedInputDeviceId = currentValue;
                }
                Connections {
                    target: audioSettingsModel
                    function onInputDevicesChanged() {
                        audioInputDeviceComboBox.currentIndex = audioInputDeviceComboBox.indexOfValue(audioSettingsModel.selectedInputDeviceId);
                    }
                }
            }
            Button {
                text: qsTr("Refresh")
                Layout.column: 4
                Layout.row: 5
                enabled: enableAudioRecordingCheckbox.checked && !settingsService.jackSyncEnabled
                onClicked: audioSettingsModel.refreshInputDevices()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Refresh input device list")
            }

            Label {
                text: qsTr("Output Device:")
                Layout.column: 0
                Layout.columnSpan: 2
                Layout.row: 6
                Layout.fillWidth: true
            }
            ComboBox {
                id: audioOutputDeviceComboBox
                Layout.column: 3
                Layout.row: 6
                Layout.fillWidth: true
                enabled: enableAudioRecordingCheckbox.checked && !settingsService.jackSyncEnabled
                model: audioSettingsModel.outputDevices
                textRole: "name"
                valueRole: "id"
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: settingsService.jackSyncEnabled ? qsTr("Device selection is managed by JACK routing") : qsTr("Select audio output device")
                Component.onCompleted: {
                    currentIndex = indexOfValue(audioSettingsModel.selectedOutputDeviceId);
                }
                onActivated: {
                    audioSettingsModel.selectedOutputDeviceId = currentValue;
                }
                Connections {
                    target: audioSettingsModel
                    function onOutputDevicesChanged() {
                        audioOutputDeviceComboBox.currentIndex = audioOutputDeviceComboBox.indexOfValue(audioSettingsModel.selectedOutputDeviceId);
                    }
                }
            }
            Button {
                text: qsTr("Refresh")
                Layout.column: 4
                Layout.row: 6
                enabled: enableAudioRecordingCheckbox.checked && !settingsService.jackSyncEnabled
                onClicked: audioSettingsModel.refreshOutputDevices()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Refresh output device list")
            }
        }
    }
}
