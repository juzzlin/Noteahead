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
import QtQuick.Dialogs

import Noteahead 1.0
import "../Components"

Dialog {
    id: root
    title: "<strong>" + qsTr("Sampler") + "</strong>"
    modal: true
    focus: true

    onAboutToShow: {
        samplerController.initialize()
        waveform.updateWaveform()
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: {
            samplerController.accept()
            root.accept()
        }
        onRejected: {
            samplerController.reject()
            root.reject()
        }
    }

    background: Rectangle {
        color: "#222"
        border.color: "#444"
        radius: 10
    }

    FileDialog {
        id: sampleFileDialog
        title: qsTr("Select Sample")
        nameFilters: [qsTr("Audio files (*.wav *.WAV)")]
        property int padToAssign: -1
        onAccepted: {
            if (padToAssign !== -1) {
                samplerController.loadSample(padToAssign, selectedFile.toString().replace("file://", ""))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.preferredWidth: resetBtn.width }
            Text {
                text: qsTr("Noteahead Sampler")
                color: "white"
                font.bold: true
                font.pointSize: 20
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                id: resetBtn
                text: qsTr("Reset")
                implicitWidth: Constants.defaultButtonWidth
                onClicked: samplerController.reset()
            }
        }

        Text {
            text: qsTr("Press and hold pad to play, release to stop. Right-click to clear. Assignments are saved with the song project. To use the sampler, select 'Noteahead Sampler' as the port in Track Settings.")
            color: "#aaa"
            font.pointSize: 10
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }

        WaveformView {
            id: waveform
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            Layout.margins: 10
            
            property var currentWaveformData: []
            waveformData: currentWaveformData
            fileName: {
                if (samplerController.selectedPad < 0) return "";
                const sample = samplerController.padModel.data(samplerController.padModel.index(samplerController.selectedPad, 0), SamplerPadModel.FilePath);
                return sample ? sample.split("/").pop() : "";
            }

            playbackPosition: samplerController.playbackPosition
            startOffset: {
                if (samplerController.selectedPadDuration > 0) {
                    return (samplerController.selectedPadStartOffsetSeconds + samplerController.selectedPadStartOffsetMilliseconds / 1000.0) / samplerController.selectedPadDuration;
                }
                return 0.0;
            }
            showPlayhead: fileName !== ""

            Timer {
                interval: 20
                running: root.visible && waveform.fileName !== ""
                repeat: true
                onTriggered: {
                    samplerController.updatePlaybackStatus()
                }
            }

            function updateWaveform() {
                if (width > 0) {
                    const data = samplerController.getWaveformData(width - 12);
                    currentWaveformData = data || [];
                }
            }

            onWidthChanged: updateWaveform()
            
            Connections {
                target: samplerController
                function onSelectedPadChanged() {
                    waveform.updateWaveform();
                }
            }
            
            Connections {
                target: samplerController.padModel
                function onDataChanged() {
                    waveform.updateWaveform();
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            GridView {
                id: padGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width * 0.66
                cellWidth: width / 4
                cellHeight: height / 4
                model: samplerController.padModel
                interactive: false

                delegate: Item {
                    width: padGrid.cellWidth
                    height: padGrid.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 8
                        radius: 12
                        color: isLoaded ? "#228822" : "#882222"
                        border.color: samplerController.selectedPad === index ? themeService.accentColor : (mouseArea.pressed ? "white" : "#555")
                        border.width: samplerController.selectedPad === index ? 3 : 2

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Text {
                                text: "Note: " + noteName + " (" + note + ")"
                                color: "white"
                                font.bold: true
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Text {
                                text: isLoaded ? "LOADED" : "EMPTY"
                                color: "white"
                                font.pointSize: 8
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            hoverEnabled: true

                            ToolTip.delay: Constants.toolTipDelay
                            ToolTip.timeout: Constants.toolTipTimeout
                            ToolTip.visible: isLoaded && containsMouse
                            ToolTip.text: filePath

                            onPressed: (mouse) => {
                                samplerController.selectedPad = index;
                                if (mouse.button === Qt.LeftButton) {
                                    if (isLoaded) {
                                        samplerController.playSample(index, 1.0)
                                    } else {
                                        sampleFileDialog.padToAssign = index
                                        sampleFileDialog.open()
                                    }
                                }
                            }

                            onReleased: (mouse) => {
                                if (mouse.button === Qt.LeftButton && isLoaded) {
                                    samplerController.stopSample(index)
                                }
                            }

                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    if (isLoaded) {
                                        samplerController.clearSample(index)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width * 0.33
                Layout.alignment: Qt.AlignTop
                spacing: 15

                component ThemedSlider : Slider {
                    id: control
                    Layout.fillWidth: true
                    handle: Rectangle {
                        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
                        y: control.topPadding + control.availableHeight / 2 - height / 2
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 8
                        color: control.pressed ? "#f0f0f0" : "#f6f6f6"
                        border.color: themeService.accentColor
                    }
                    background: Rectangle {
                        x: control.leftPadding
                        y: control.topPadding + control.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 4
                        width: control.availableWidth
                        height: implicitHeight
                        radius: 2
                        color: "#444"

                        Rectangle {
                            width: control.visualPosition * parent.width
                            height: parent.height
                            color: themeService.accentColor
                            radius: 2
                        }
                    }
                }

                // Pan Slider
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Pan: ") + (panSlider.value > 0 ? "+" : "") + Math.round(panSlider.value) + "%"
                        color: "white"
                    }
                    ThemedSlider {
                        id: panSlider
                        from: -100
                        to: 100
                        stepSize: 1
                        value: (samplerController.selectedPadPan * 200) - 100
                        onMoved: {
                            samplerController.selectedPadPan = (value + 100) / 200
                        }
                    }
                }

                // Volume Slider
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Volume: ") + Math.round(volumeSlider.value) + "%"
                        color: "white"
                    }
                    ThemedSlider {
                        id: volumeSlider
                        from: 0
                        to: 100
                        stepSize: 1
                        value: samplerController.selectedPadVolume * 100
                        onMoved: {
                            samplerController.selectedPadVolume = value / 100
                        }
                    }
                }

                // LPF Cutoff Slider
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("LPF Cutoff: ") + Math.round(cutoffSlider.value) + "%"
                        color: "white"
                    }
                    ThemedSlider {
                        id: cutoffSlider
                        from: 0
                        to: 100
                        stepSize: 1
                        value: samplerController.selectedPadCutoff * 100
                        onMoved: {
                            samplerController.selectedPadCutoff = value / 100
                        }
                    }
                }

                // HPF Cutoff Slider
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("HPF Cutoff: ") + Math.round(hpfCutoffSlider.value) + "%"
                        color: "white"
                    }
                    ThemedSlider {
                        id: hpfCutoffSlider
                        from: 0
                        to: 100
                        stepSize: 1
                        value: samplerController.selectedPadHpfCutoff * 100
                        onMoved: {
                            samplerController.selectedPadHpfCutoff = value / 100
                        }
                    }
                }

                // Start Offset
                ColumnLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Start Offset:")
                        color: "white"
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        SpinBox {
                            id: secondsSpinBox
                            Layout.fillWidth: true
                            from: 0
                            to: 3600
                            value: samplerController.selectedPadStartOffsetSeconds
                            editable: true
                            onValueModified: samplerController.selectedPadStartOffsetSeconds = value
                            Keys.onReturnPressed: {
                                value = valueFromText(contentItem.text, locale)
                                samplerController.selectedPadStartOffsetSeconds = value
                            }
                        }
                        Label { text: "s"; color: "white" }
                        SpinBox {
                            id: msSpinBox
                            Layout.fillWidth: true
                            from: 0
                            to: 999
                            value: samplerController.selectedPadStartOffsetMilliseconds
                            editable: true
                            onValueModified: samplerController.selectedPadStartOffsetMilliseconds = value
                            Keys.onReturnPressed: {
                                value = valueFromText(contentItem.text, locale)
                                samplerController.selectedPadStartOffsetMilliseconds = value
                            }
                        }
                        Label { text: "ms"; color: "white" }
                    }
                }
            }
        }

        CheckBox {
            id: channelModeCheckbox
            text: qsTr("Map pads to MIDI channels 1-16 (for MIDI CC automation only)")
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 20
            checked: samplerController.channelMode
            onToggled: samplerController.channelMode = checked
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("When enabled, MIDI CCs on channel 1 affect pad 1, channel 2 affects pad 2, and so on. Note playing is unaffected.")
        }
    }
}
