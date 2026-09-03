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

    footer: DialogButtonBox {
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
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        SamplerDialog_Header {
            Layout.fillWidth: true
        }

        SamplerDialog_WaveformView {
            id: waveform
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
                    fileDialog: sampleFileDialog
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop
                }

                SamplerDialog_PadSettings {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.22
                    Layout.alignment: Qt.AlignTop
                }

                SamplerDialog_PadAmpEg {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width * 0.22
                    Layout.alignment: Qt.AlignTop
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
