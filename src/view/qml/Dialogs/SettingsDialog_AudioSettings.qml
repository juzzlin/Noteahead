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

        GroupBox {
            title: qsTr("General")
            Layout.fillWidth: true
            CheckBox {
                id: showWaveViewCheckbox
                text: qsTr("Show recording and playback wave view at the bottom of the editor.")
                checked: settingsService.waveViewEnabled
                anchors.fill: parent
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
        }

        GroupBox {
            title: qsTr("Input")
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                CheckBox {
                    id: enableAudioRecordingCheckbox
                    text: qsTr("Enable audio recording when playing.\nAudio files will appear next to the current project file.")
                    checked: settingsService.recordingEnabled
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

                RowLayout {
                    spacing: 10
                    Label {
                        text: qsTr("Buffer size (samples):")
                    }
                    SpinBox {
                        id: audioBufferSizeSpinBox
                        from: 32
                        to: 4096
                        stepSize: 32
                        enabled: !settingsService.jackSyncEnabled
                        value: settingsService.audioBufferSize()
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: settingsService.jackSyncEnabled ? qsTr("Buffer size is managed by JACK server") : qsTr("Set buffer size for audio recording and playback")
                        onValueChanged: settingsService.setAudioBufferSize(value)
                        Keys.onReturnPressed: focus = false
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    enabled: enableAudioRecordingCheckbox.checked
                    Label {
                        text: qsTr("Device:")
                        Layout.fillWidth: true
                    }
                    ComboBox {
                        id: audioInputDeviceComboBox
                        Layout.fillWidth: true
                        enabled: !settingsService.jackSyncEnabled
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
                        enabled: !settingsService.jackSyncEnabled
                        onClicked: audioSettingsModel.refreshInputDevices()
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Refresh input device list")
                    }
                }
            }
        }

        GroupBox {
            title: qsTr("Output")
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 3
                Label {
                    text: qsTr("Device:")
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: audioOutputDeviceComboBox
                    Layout.fillWidth: true
                    enabled: !settingsService.jackSyncEnabled
                    model: audioOutputDeviceComboBox.enabled ? audioSettingsModel.outputDevices : []
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
                    enabled: !settingsService.jackSyncEnabled
                    onClicked: audioSettingsModel.refreshOutputDevices()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Refresh output device list")
                }
            }
        }
    }
}
