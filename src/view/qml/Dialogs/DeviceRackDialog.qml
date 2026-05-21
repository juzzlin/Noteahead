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
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"

Dialog {
    id: root
    title: "<strong>" + qsTr("Device Rack") + "</strong>"
    modal: true
    focus: true
    width: 600
    height: 500

    function updateUsage(): void {
        deviceRackController.refresh();
    }

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Label {
            text: qsTr("Device Rack")
            font.bold: true
            font.pointSize: 18
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: deviceListView
            model: deviceRackController.deviceCount
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: deviceListView.width
                height: 60
                color: (deviceListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                readonly property string deviceType: {
                    deviceRackController.revision;
                    return deviceRackController.deviceType(index);
                }
                readonly property string deviceTypeName: {
                    deviceRackController.revision;
                    return deviceRackController.deviceTypeName(index);
                }
                readonly property string deviceName: {
                    deviceRackController.revision;
                    return deviceRackController.deviceName(index);
                }
                readonly property string trackNames: {
                    deviceRackController.revision;
                    return deviceRackController.trackNames(index);
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: deviceListView.hoveredIndex = index
                    onExited: deviceListView.hoveredIndex = -1
                    onClicked: {
                        deviceListView.hoveredIndex = -1;
                        if (deviceType === "") {
                            UiService.requestDeviceGalleryDialog(index);
                        } else {
                            deviceRackController.openDevice(index);
                        }
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    
                    Text {
                        text: deviceType === "" ? "" : qsTr("Slot %1: %2 (%3)").arg(index + 1).arg(deviceName).arg(deviceTypeName)
                        color: "white"
                        font.pointSize: 13
                        font.bold: deviceListView.hoveredIndex === index && root.activeFocus
                        Layout.fillWidth: true
                        visible: deviceType !== ""
                    }
                    
                    Text {
                        text: trackNames
                        color: "#aaa"
                        font.pointSize: 11
                        Layout.preferredWidth: 150
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                        visible: deviceType !== ""
                    }

                    Image {
                        source: "../Graphics/add_box.png"
                        sourceSize.width: 32
                        sourceSize.height: 32
                        Layout.alignment: Qt.AlignCenter
                        visible: deviceType === ""
                        opacity: (deviceListView.hoveredIndex === index && root.activeFocus) ? 1.0 : 0.5
                    }

                    Button {
                        text: qsTr("Insert FX")
                        onClicked: UiService.requestDeviceInsertEffectsDialog(deviceName)
                        Layout.preferredWidth: 80
                        visible: deviceType !== ""
                    }

                    Button {
                        text: qsTr("Sends")
                        onClicked: UiService.requestEffectSendsDialog(deviceName)
                        Layout.preferredWidth: 80
                        visible: deviceType !== ""
                    }

                    Button {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        visible: deviceType !== ""
                        flat: true
                        padding: 0
                        Image {
                            source: "../Graphics/delete.png"
                            anchors.fill: parent
                            anchors.margins: 4
                            sourceSize.width: 24
                            sourceSize.height: 24
                            fillMode: Image.PreserveAspectFit
                            opacity: parent.hovered ? 1.0 : 0.6
                        }
                        onClicked: deviceRackController.clearDevice(index)
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
