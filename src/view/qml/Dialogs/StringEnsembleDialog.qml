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
    title: applicationService.stringEnsembleDeviceName
    modal: true
    focus: true
    width: 1150
    height: 700
    clip: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    onAboutToShow: () => {
        stringEnsembleController.requestSettings();
    }

    // Cancel here throws away every edit made in the dialog, so the buttons take the whole
    // footer width instead of sitting side by side in the corner
    stretchFooterButtons: true

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: () => {
            stringEnsembleController.accept();
        }
        onRejected: () => {
            stringEnsembleController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            // Fixed Sidebar: Global settings
            ColumnLayout {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: globalScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StringEnsembleDialog_Global {
                        width: globalScrollView.availableWidth
                    }
                }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Register switches
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: registersScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StringEnsembleDialog_Registers {
                        width: registersScrollView.availableWidth
                    }
                }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Envelope, Modulation and Phaser
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: envelopeScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StringEnsembleDialog_Envelope {
                        width: envelopeScrollView.availableWidth
                    }
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => stringEnsembleController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => stringEnsembleController.stopNote(note)
        }
    }
}
