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

Dialog {
    id: root
    title: "<strong>" + qsTr("Effects Gallery") + "</strong>"
    modal: true
    focus: true
    width: 400
    height: 400

    property int slotIndex: -1

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
            text: qsTr("Select Effect")
            font.bold: true
            font.pointSize: 16
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: effectListView
            model: effectRackController.availableEffects()
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: effectListView.width
                height: 50
                color: (effectListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: effectListView.hoveredIndex = index
                    onExited: effectListView.hoveredIndex = -1
                    onClicked: {
                        effectListView.hoveredIndex = -1;
                        effectRackController.setEffect(root.slotIndex, modelData.typeId);
                        root.accept();
                    }
                }
                Text {
                    anchors.centerIn: parent
                    text: modelData.name
                    color: "white"
                    font.pointSize: 12
                    font.bold: effectListView.hoveredIndex === index && root.activeFocus
                }
            }
        }

        Button {
            text: qsTr("Clear Slot")
            Layout.fillWidth: true
            visible: root.slotIndex !== -1 && effectRackController.effectType(root.slotIndex) !== ""
            onClicked: {
                effectRackController.clearEffect(root.slotIndex);
                root.accept();
            }
        }
    }
}
