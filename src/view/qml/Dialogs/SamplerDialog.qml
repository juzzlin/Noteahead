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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import Noteahead

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: 500
    height: 600
    anchors.centerIn: Overlay.overlay

    background: Rectangle {
        color: "#222"
        border.color: "#444"
        radius: 10
    }

    FileDialog {
        id: sampleFileDialog
        title: qsTr("Select Sample")
        nameFilters: [qsTr("Audio files (*.wav *.WAV)")]
        property int noteToAssign: -1
        onAccepted: {
            if (noteToAssign !== -1) {
                samplerModel.loadSample(noteToAssign, selectedFile.toString().replace("file://", ""))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        Text {
            text: qsTr("Noteahead Sampler")
            color: "white"
            font.bold: true
            font.pointSize: 20
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: qsTr("Click to assign/play, right-click to clear")
            color: "#aaa"
            font.pointSize: 10
            Layout.alignment: Qt.AlignHCenter
        }

        GridView {
            id: padGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth: width / 4
            cellHeight: height / 4
            model: samplerModel
            interactive: false

            delegate: Item {
                width: padGrid.cellWidth
                height: padGrid.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 8
                    radius: 12
                    color: isLoaded ? "#228822" : "#882222"
                    border.color: mouseArea.pressed ? "white" : "#555"
                    border.width: 2

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            text: note
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
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                if (isLoaded) {
                                    samplerModel.clearSample(index)
                                }
                            } else {
                                if (isLoaded) {
                                    samplerModel.playSample(index, 1.0)
                                } else {
                                    sampleFileDialog.noteToAssign = index
                                    sampleFileDialog.open()
                                }
                            }
                        }
                    }
                }
            }
        }

        Button {
            text: qsTr("Close")
            Layout.alignment: Qt.AlignRight
            onClicked: root.close()
        }
    }
}
