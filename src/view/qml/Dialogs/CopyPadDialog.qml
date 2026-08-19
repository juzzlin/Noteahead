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
    title: "<strong>" + qsTr("Copy Pad") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    // The pad the copy is written to.
    property int padIndex: -1

    // Populated when the dialog is opened so the list reflects the current pads.
    property var pads: []

    onOpened: pads = samplerController.loadedPads()

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
            text: qsTr("Copy From Pad")
            font.bold: true
            font.pointSize: 16
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("No pads to copy.")
            color: "#aaa"
            font.pointSize: 12
            visible: root.pads.length === 0 || (root.pads.length === 1 && root.pads[0].padIndex === root.padIndex)
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: padListView
            model: root.pads
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: padListView.width
                height: 50
                // The target pad itself is not a valid copy source.
                visible: modelData.padIndex !== root.padIndex
                color: (padListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: padListView.hoveredIndex = index
                    onExited: padListView.hoveredIndex = -1
                    onClicked: {
                        padListView.hoveredIndex = -1;
                        samplerController.copyPad(modelData.padIndex, root.padIndex);
                        root.accept();
                    }
                }
                Text {
                    anchors.centerIn: parent
                    text: qsTr("%1 (%2): %3").arg(modelData.noteName).arg(modelData.note).arg(modelData.fileName)
                    color: "white"
                    font.pointSize: 12
                    font.bold: padListView.hoveredIndex === index && root.activeFocus
                }
            }
        }
    }
}
