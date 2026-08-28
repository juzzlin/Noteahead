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
    title: "<strong>" + qsTr("Copy Effect Rack") + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    // Name of the rack being copied into, for the caption. The controller already targets it.
    property string targetName: ""

    // Populated when the dialog is opened so the list reflects the current device rack. The rack
    // being copied into is left out by the controller.
    property var sources: []

    onOpened: sources = effectRackController.availableRackSources()

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
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
            text: root.targetName !== "" ? qsTr("Copy Into %1").arg(root.targetName) : qsTr("Copy Effect Rack")
            font.bold: true
            font.pointSize: 16
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("The selected rack replaces the current one, effects and bypass state alike.")
            color: "#aaa"
            font.pointSize: 12
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            text: qsTr("No racks to copy.")
            color: "#aaa"
            font.pointSize: 12
            visible: root.sources.length === 0
            Layout.alignment: Qt.AlignHCenter
        }

        ListView {
            id: sourceListView
            model: root.sources
            property int hoveredIndex: -1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 10
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                width: sourceListView.width
                height: 50
                color: (sourceListView.hoveredIndex === index && root.activeFocus) ? themeService.accentColor : "#333"
                radius: 5
                border.color: "#555"
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: sourceListView.hoveredIndex = index
                    onExited: sourceListView.hoveredIndex = -1
                    onClicked: {
                        sourceListView.hoveredIndex = -1;
                        effectRackController.copyRackFrom(modelData.deviceName, modelData.isInsertRack);
                        root.accept();
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10
                    Text {
                        text: modelData.description !== "" ? qsTr("%1 (%2)").arg(modelData.name).arg(modelData.description) : modelData.name
                        color: "white"
                        font.pointSize: 12
                        font.bold: sourceListView.hoveredIndex === index && root.activeFocus
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        // An empty rack is a valid choice -- it is how one clears a rack in one go --
                        // so it is worth saying which ones are empty before they are picked.
                        text: modelData.effectCount === 0 ? qsTr("empty") : qsTr("%n effect(s)", "", modelData.effectCount)
                        color: "#aaa"
                        font.pointSize: 11
                        horizontalAlignment: Text.AlignRight
                        Layout.preferredWidth: 100
                    }
                }
            }
        }
    }
}
