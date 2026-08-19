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

AnimatedDialog {
    id: root
    title: "<strong>" + qsTr("Copy Device") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    property int slotIndex: -1

    // Populated when the dialog is opened so the list reflects the current device rack.
    property var devices: []

    onOpened: devices = deviceRackController.populatedDevices()

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        AppButton {
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
            text: qsTr("Copy From Device")
            font.bold: true
            font.pointSize: 16
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("No devices to copy.")
            color: "#aaa"
            font.pointSize: 12
            visible: root.devices.length === 0 || (root.devices.length === 1 && root.devices[0].slotIndex === root.slotIndex)
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: deviceListView
            model: root.devices
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: deviceListView.width
                height: 50
                // The target slot itself is not a valid copy source.
                visible: modelData.slotIndex !== root.slotIndex
                color: (deviceListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: deviceListView.hoveredIndex = index
                    onExited: deviceListView.hoveredIndex = -1
                    onClicked: {
                        deviceListView.hoveredIndex = -1;
                        deviceRackController.copyDevice(modelData.slotIndex, root.slotIndex);
                        root.accept();
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    Text {
                        text: modelData.name + " (" + modelData.typeName + ")"
                        color: "white"
                        font.pointSize: 12
                        font.bold: deviceListView.hoveredIndex === index && root.activeFocus
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        text: modelData.trackNames
                        color: "#aaa"
                        font.pointSize: 11
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                        visible: modelData.trackNames !== ""
                        Layout.preferredWidth: 200
                    }
                }
            }
        }
    }
}
