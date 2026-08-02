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
import QtQuick.Dialogs
import QtQuick.Layouts
import ".."

GroupBox {
    title: qsTr("Theme")
    label: Label {
        text: parent.title
        color: themeService.mainMenuTextColor
        font.bold: true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        GroupBox {
            title: qsTr("Colors")
            Layout.fillWidth: true
            label: Label {
                text: parent.title
                color: themeService.mainMenuTextColor
                font.bold: true
            }

            RowLayout {
                spacing: 20
                width: parent.width
                Label {
                    text: qsTr("Accent color:")
                    Layout.fillWidth: true
                    color: themeService.mainMenuTextColor
                }
                Rectangle {
                    id: colorPreview
                    width: 40
                    height: 20
                    color: themeService.accentColor
                    border.color: themeService.mainMenuTextColor
                    border.width: 1
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: colorDialog.open()
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Select Accent Color")
        selectedColor: themeService.accentColor
        onAccepted: {
            themeService.accentColor = selectedColor
        }
    }
}
