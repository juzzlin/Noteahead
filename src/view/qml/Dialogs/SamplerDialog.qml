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

AnimatedDialog {
    id: root
    title: "<strong>" + applicationService.samplerDeviceName + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 1000
    height: parent ? parent.height * Constants.largeDialogScale : 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    onAboutToShow: {
        samplerController.initialize();
        waveform.updateWaveform();
    }
    // The input list is not asked for here. Enumerating audio devices means probing the hardware,
    // which blocks whoever asks, and everything in onAboutToShow runs before the dialog has painted a
    // single frame -- so the wait landed on a window that was not there yet, and there was nothing to
    // put a spinner on. The record strip asks once it is on screen instead.
    onOpened: recordStrip.refreshInputs()

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: samplerController
            hostDialog: root
            // Reset role: the leftmost group of the box, and like Action it leaves the dialog open
            DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: {
            samplerController.accept();
            root.accept();
        }
        onRejected: {
            samplerController.reject();
            root.reject();
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
        nameFilters: [qsTr("Audio files") + " (*.wav *.WAV *.flac *.FLAC)", qsTr("WAV files") + " (*.wav *.WAV)", qsTr("FLAC files") + " (*.flac *.FLAC)"]
        property int padToAssign: -1
        onAccepted: {
            if (padToAssign !== -1) {
                samplerController.loadSample(padToAssign, selectedFile.toString().replace("file://", ""));
            }
        }
    }

    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        SamplerDialog_Header {
            Layout.fillWidth: true
        }

        // The wave and the meters read as one strip: the meters are about the signal the wave is a
        // picture of.
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            SamplerDialog_WaveformView {
                id: waveform
                Layout.fillWidth: true
                samplerDialogVisible: root.visible
            }

            SamplerDialog_Meters {
                // Matched to the wave view's own height rather than filling, so the bars stand beside
                // the picture instead of the margins around it.
                Layout.preferredHeight: waveform.Layout.preferredHeight
                Layout.alignment: Qt.AlignVCenter
                samplerDialogVisible: root.visible
            }
        }

        SamplerDialog_Record {
            id: recordStrip
            Layout.fillWidth: true
            samplerDialogVisible: root.visible
        }

        ScrollView {
            id: bottomScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: -1

            RowLayout {
                width: bottomScrollView.availableWidth
                height: Math.max(implicitHeight, bottomScrollView.availableHeight)
                spacing: 20

                SamplerDialog_Pads {
                    id: pads
                    fileDialog: sampleFileDialog
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                }

                // Vertical Separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }

                SamplerDialog_PadSettings {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.22
                    Layout.alignment: Qt.AlignTop
                }

                // Vertical Separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }

                SamplerDialog_PadAmpEg {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.22
                    Layout.alignment: Qt.AlignTop
                }

                // Vertical Separator
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }

                SamplerDialog_Global {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.22
                    Layout.alignment: Qt.AlignTop
                }

                Item {
                    Layout.preferredWidth: 15
                }
            }
        }
    }
}
