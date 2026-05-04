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

import Noteahead 1.0
import "../Components"

Dialog {
    id: root
    title: "<strong>" + qsTr("Device Rack") + "</strong>"
    modal: true
    focus: true

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    background: Rectangle {
        color: "#222"
        border.color: "#444"
        radius: 10
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 20

        Text {
            text: qsTr("Device Rack")
            color: "white"
            font.bold: true
            font.pointSize: 18
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            columns: 1
            rowSpacing: 10
            Layout.fillWidth: true

            Repeater {
                model: deviceRackController.devices
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: 15
                    
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        color: mouseArea.containsMouse ? themeService.accentColor : "#333"
                        radius: 5
                        border.color: "#555"
                        
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: deviceRackController.openDevice(modelData)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 15
                            
                            Text {
                                text: modelData
                                color: "white"
                                font.pointSize: 13
                                font.bold: mouseArea.containsMouse
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }
}
