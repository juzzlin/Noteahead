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
    title: "<strong>" + qsTr("Device Gallery") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    property int slotIndex: -1

    // Read when the dialog opens rather than bound: picking a device replaces the slot's device,
    // which would otherwise re-evaluate the name under the label that is naming what is going.
    property string replacedDeviceTypeName: ""

    onOpened: replacedDeviceTypeName = root.slotIndex !== -1 ? deviceRackController.deviceTypeName(root.slotIndex) : ""

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Import...")
            implicitWidth: Constants.defaultButtonWidth
            visible: root.slotIndex !== -1
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                root.accept();
                UiService.requestImportDeviceSettings(root.slotIndex);
            }
        }
        AppButton {
            text: qsTr("Copy...")
            implicitWidth: Constants.defaultButtonWidth
            visible: root.slotIndex !== -1
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                root.accept();
                UiService.requestCopyDeviceDialog(root.slotIndex);
            }
        }
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Label {
            text: qsTr("Select Device")
            font.bold: true
            font.pointSize: 16
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("Replacing %1 — insert effects, sends and mixer settings are kept.").arg(root.replacedDeviceTypeName)
            color: "#aaa"
            font.pointSize: 12
            visible: root.replacedDeviceTypeName !== ""
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ListView {
            id: deviceListView
            model: deviceRackController.availableDevices()
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: deviceListView.width
                height: 50
                color: (deviceListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: deviceListView.hoveredIndex = index
                    onExited: deviceListView.hoveredIndex = -1
                    onClicked: {
                        deviceListView.hoveredIndex = -1;
                        deviceRackController.replaceDevice(root.slotIndex, modelData.typeId);
                        root.accept();
                    }
                }
                Text {
                    anchors.centerIn: parent
                    text: modelData.name
                    color: "white"
                    font.pointSize: 12
                    font.bold: deviceListView.hoveredIndex === index && root.activeFocus
                }
            }
        }

        AppButton {
            text: qsTr("Clear Slot")
            Layout.fillWidth: true
            visible: root.slotIndex !== -1 && deviceRackController.deviceType(root.slotIndex) !== ""
            onClicked: {
                deviceRackController.clearDevice(root.slotIndex);
                root.accept();
            }
        }
    }
}
