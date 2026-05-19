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
    function updateUsage(): void {
        deviceRackController.refresh();
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
        ListView {
            id: deviceListView
            model: deviceRackController
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 400
            Layout.preferredWidth: 600
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Item {
                width: deviceListView.width
                height: 60
                
                RowLayout {
                    anchors.fill: parent
                    spacing: 15
                    
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: mouseArea.containsMouse ? themeService.accentColor : "#333"
                        radius: 5
                        border.color: "#555"
                        
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: deviceRackController.openDevice(model.name)
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 15
                            anchors.verticalCenter: parent.verticalCenter
                            text: model.name
                            color: "white"
                            font.pointSize: 13
                            font.bold: mouseArea.containsMouse
                        }
                    }

                    Button {
                        text: qsTr("Sends")
                        onClicked: UiService.requestEffectSendsDialog(model.name)
                        Layout.preferredWidth: 80
                    }

                    Text {
                        text: model.trackNames
                        color: "#aaa"
                        font.pointSize: 11
                        Layout.preferredWidth: 150
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Text {
            text: qsTr("To assign an internal device to a track, select its name from the port list in the Track Settings dialog.")
            color: "#aaa"
            font.italic: true
            font.pointSize: 11
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
