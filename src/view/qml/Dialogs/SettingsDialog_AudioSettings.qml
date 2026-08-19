// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."
import "../Components"

GroupBox {
    title: qsTr("Audio")
    anchors.left: parent.left
                anchors.right: parent.right
    ColumnLayout {
        anchors.left: parent.left
                anchors.right: parent.right
        spacing: 10
        enabled: !UiService.isPlaying()
        opacity: enabled ? 1.0 : 0.5

        GroupBox {
            title: qsTr("General")
            Layout.fillWidth: true
            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 10
                RowLayout {
                    spacing: 10
                    Label {
                        text: qsTr("Backend:")
                    }
                    ComboBox {
                        id: audioBackendComboBox
                        Layout.fillWidth: true
                        model: [qsTr("Auto"), qsTr("ALSA"), qsTr("PulseAudio"), qsTr("JACK")]
                        currentIndex: settingsService.audioBackend
                        onActivated: settingsService.audioBackend = index
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Select audio backend")
                    }
                }

                RowLayout {
                    spacing: 10
                    Label {
                        text: qsTr("Playback quality:")
                    }
                    ComboBox {
                        id: playbackQualityComboBox
                        Layout.fillWidth: true
                        readonly property var factors: [1, 2, 4]
                        model: [qsTr("Draft (1x)"), qsTr("Normal (2x)"), qsTr("High (4x)")]
                        currentIndex: Math.max(0, factors.indexOf(settingsService.playbackOversampleFactor))
                        onActivated: index => settingsService.playbackOversampleFactor = factors[index]
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Oversampling factor for realtime playback. Lower saves CPU; higher reduces aliasing in the internal synths.")
                    }
                }

                CheckBox {
                    id: multiThreadedPlaybackCheckbox
                    text: qsTr("Spread playback across CPU cores.")
                    checked: settingsService.multiThreadedPlaybackEnabled
                    Layout.fillWidth: true
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Processes devices on several threads during playback. Needs real-time scheduling privileges; without them playback stays single-threaded.")
                    onCheckedChanged: {
                        if (settingsService.multiThreadedPlaybackEnabled !== checked) {
                            settingsService.multiThreadedPlaybackEnabled = checked
                        }
                    }
                }

                CheckBox {
                    id: showWaveViewCheckbox
                    text: qsTr("Show recording and playback wave view at the bottom of the editor.")
                    checked: settingsService.waveViewEnabled
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
            }
        }

        GroupBox {
            title: qsTr("Input")
            Layout.fillWidth: true
            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
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
                        enabled: settingsService.audioBackend !== 3
                        value: settingsService.audioBufferSize()
                        Layout.fillWidth: true
                        editable: true
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: settingsService.audioBackend === 3 ? qsTr("Buffer size is managed by JACK server") : qsTr("Set buffer size for audio recording and playback")
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
                        enabled: settingsService.audioBackend !== 3
                        model: audioSettingsModel.inputDevices
                        textRole: "name"
                        valueRole: "id"
                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: hovered
                        ToolTip.text: settingsService.audioBackend === 3 ? qsTr("Device selection is managed by JACK routing") : qsTr("Select audio input device")
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
                    AppButton {
                        text: qsTr("Refresh")
                        enabled: settingsService.audioBackend !== 3
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
                anchors.left: parent.left
                anchors.right: parent.right
                columns: 3
                Label {
                    text: qsTr("Device:")
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: audioOutputDeviceComboBox
                    Layout.fillWidth: true
                    enabled: settingsService.audioBackend !== 3
                    model: audioOutputDeviceComboBox.enabled ? audioSettingsModel.outputDevices : []
                    textRole: "name"
                    valueRole: "id"
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: settingsService.audioBackend === 3 ? qsTr("Device selection is managed by JACK routing") : qsTr("Select audio output device")
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
                AppButton {
                    text: qsTr("Refresh")
                    enabled: settingsService.audioBackend !== 3
                    onClicked: audioSettingsModel.refreshOutputDevices()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Refresh output device list")
                }
            }
        }
    }
}
