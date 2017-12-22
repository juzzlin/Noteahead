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
import ".."
import "../Components"

AnimatedDialog {
    id: rootItem
    title: qsTr("Open a recent project")
    modal: true
    signal fileSelected(string filePath)
    property string selectedFile
    // Starts over every time, so the field never hides a file the user is about to look for
    onAboutToShow: {
        filterField.text = "";
        recentFilesModel.filter = "";
    }
    onOpened: filterField.forceActiveFocus()
    function openFile(filePath: string): void {
        recentFilesDialog.close();
        recentFilesDialog.selectedFile = filePath;
        recentFilesDialog.fileSelected(filePath);
    }
    SectionBox {
        title: qsTr("Recent files")
        anchors.fill: parent
        clip: true
        ScrollView {
            anchors.fill: parent
            clip: true
            ListView {
                id: recentFilesList
                model: recentFilesModel
                anchors.fill: parent
                delegate: Item {
                    id: recentFileItem
                    width: recentFilesList.width
                    height: recentFileText.height + 20
                    Label {
                        id: recentFileText
                        text: model.filePath
                        font.strikeout: !model.exists
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        elide: Text.ElideRight
                        color: hoverHandler.hovered ? themeService.accentColor : themeService.recentFileItemTextColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: model.exists
                        onClicked: rootItem.openFile(model.filePath)
                        HoverHandler {
                            id: hoverHandler
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }
        }
    }
    // The filter shares the row with Cancel, so the list keeps the whole height
    footer: RowLayout {
        spacing: 0
        TextField {
            id: filterField
            placeholderText: qsTr("Filter by name or folder")
            Layout.fillWidth: true
            Layout.leftMargin: 24
            // Centred on Cancel, which the box's padding keeps off the row's centre
            Layout.topMargin: buttonBox.topPadding - buttonBox.bottomPadding
            selectByMouse: true
            onTextChanged: recentFilesModel.filter = text
            // Enter opens the first match, so typing a few letters of a name is enough
            onAccepted: {
                const filePath = recentFilesModel.firstExistingFile();
                if (filePath !== "") {
                    rootItem.openFile(filePath);
                }
            }
        }
        DialogButtonBox {
            id: buttonBox
            leftPadding: 12
            Layout.preferredWidth: leftPadding + Constants.defaultButtonWidth + rightPadding
            onRejected: rootItem.reject()
            AppButton {
                text: qsTr("Cancel")
                implicitWidth: Constants.defaultButtonWidth
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Don't open any recent projects")
            }
        }
    }
}
