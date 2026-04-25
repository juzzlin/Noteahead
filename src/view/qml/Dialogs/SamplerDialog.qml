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

Dialog {
    id: root
    title: "<strong>" + qsTr("Sampler") + "</strong>"
    modal: true
    focus: true

    onAboutToShow: samplerController.initialize()

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: samplerController.accept()
        onRejected: samplerController.reject()
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

        Text {
            text: qsTr("Noteahead Sampler")
            color: "white"
            font.bold: true
            font.pointSize: 20
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: qsTr("Click to assign or play, right-click to clear. Assignments are saved with the song project. To use the sampler, select 'Noteahead Sampler' as the port in Track Settings.")
            color: "#aaa"
            font.pointSize: 10
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }

        GridView {
            id: padGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                    border.color: mouseArea.pressed ? "white" : "#555"
                    border.width: 2

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

                        ToolTip.delay: Constants.toolTipDelay
                        ToolTip.timeout: Constants.toolTipTimeout
                        ToolTip.visible: isLoaded && hovered
                        ToolTip.text: filePath

                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                if (isLoaded) {
                                    samplerController.clearSample(index)
                                }
                            } else {
                                if (isLoaded) {
                                    samplerController.playSample(index, 1.0)
                                } else {
                                    sampleFileDialog.padToAssign = index
                                    sampleFileDialog.open()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
