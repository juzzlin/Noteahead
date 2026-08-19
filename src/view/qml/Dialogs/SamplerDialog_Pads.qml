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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

GridView {
    id: padGrid
    Layout.fillHeight: true
    Layout.preferredWidth: cellWidth * 4
    implicitHeight: 400
    // Square cells sized from the available height, keeping the 4x4 grid square
    cellHeight: height / 4
    cellWidth: cellHeight
    model: samplerController.padModel
    interactive: false

    property var fileDialog

    delegate: Item {
        width: padGrid.cellWidth
        height: padGrid.cellHeight

        Rectangle {
            id: padRect
            anchors.fill: parent
            anchors.margins: 8
            radius: 12
            // On assigned pads the accent color is the background, so use a text
            // color that contrasts with it (dark on bright accents like mint).
            readonly property color textColor: isLoaded ? themeService.accentTextColor : "#888888"
            color: isLoaded ? themeService.accentColor : "#333333"
            border.color: samplerController.selectedPad === index ? themeService.accentColor : (mouseArea.pressed ? "white" : "#555")
            border.width: samplerController.selectedPad === index ? 3 : 2

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 2
                Text {
                    text: noteName + " (" + note + ")"
                    color: padRect.textColor
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: rangeLabel
                    visible: samplerController.chromaticMode && isLoaded && rangeLabel !== ""
                    color: padRect.textColor
                    font.pointSize: 8
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: isLoaded ? "LOADED" : "EMPTY"
                    color: padRect.textColor
                    font.pointSize: 8
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            AppButton {
                text: qsTr("FX")
                visible: isLoaded
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 6
                implicitWidth: 40
                implicitHeight: 26
                padding: 2
                font.pointSize: 9
                z: 10
                onClicked: UiService.requestDeviceSubEffectsDialog(samplerController.deviceName(), note, qsTr("Note %1 (%2)").arg(noteName).arg(note))
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Insert effects for this pad")
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true

                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: isLoaded && containsMouse
                ToolTip.text: filePath

                onPressed: mouse => {
                    samplerController.selectedPad = index;
                    if (mouse.button === Qt.LeftButton) {
                        if (isLoaded) {
                            samplerController.playSample(index, 1.0);
                        } else if (fileDialog) {
                            fileDialog.padToAssign = index;
                            fileDialog.open();
                        }
                    }
                }

                onReleased: mouse => {
                    if (mouse.button === Qt.LeftButton && isLoaded) {
                        samplerController.stopSample(index);
                    }
                }

                onClicked: mouse => {
                    if (mouse.button === Qt.RightButton) {
                        padMenu.popup();
                    }
                }
            }

            Menu {
                id: padMenu
                // The menu is a popup of its own, so the pad it acts on is captured here rather than
                // read off the delegate's model context from inside the items.
                readonly property int padIndex: index
                readonly property bool padIsLoaded: isLoaded
                // A popup does not inherit the theme of the dialog the pads live in either
                Universal.theme: Universal.Dark
                Universal.accent: themeService.accentColor
                delegate: MenuItemDelegate {}
                MenuItem {
                    text: qsTr("Clear")
                    enabled: padMenu.padIsLoaded
                    onTriggered: samplerController.clearSample(padMenu.padIndex)
                }
                MenuItem {
                    text: qsTr("Copy from pad...")
                    onTriggered: UiService.requestCopyPadDialog(padMenu.padIndex)
                }
            }
        }
    }
}
