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
    title: "<strong>" + qsTr("Effect Rack") + "</strong>"
    modal: true
    focus: true
    width: 600
    height: 500

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
            text: qsTr("Master Effect Rack")
            font.bold: true
            font.pointSize: 18
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: effectListView
            model: effectRackController.effectCount
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: effectListView.width
                height: 60
                color: mouseArea.containsMouse ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (effectRackController.effectType(index) === "reverb") {
                            reverbDialog.effectIndex = index;
                            reverbDialog.open();
                        }
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    Text {
                        text: qsTr("Slot %1: %2").arg(index + 1).arg(effectRackController.effectType(index).toUpperCase())
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
