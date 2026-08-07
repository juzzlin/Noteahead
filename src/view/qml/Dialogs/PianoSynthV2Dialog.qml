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
    title: applicationService.pianoSynthV2DeviceName
    modal: true
    focus: true
    width: 900
    height: 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    onAboutToShow: () => {
        pianoSynthV2Controller.requestSettings();
    }

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
            pianoSynthV2Controller.accept();
        }
        onRejected: () => {
            pianoSynthV2Controller.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            // Fixed Sidebar: Global settings
            ColumnLayout {
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                PianoSynthV2Dialog_Global {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item { Layout.fillHeight: true }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // String section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                PianoSynthV2Dialog_String {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item { Layout.fillHeight: true }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Hammer section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                PianoSynthV2Dialog_Hammer {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item { Layout.fillHeight: true }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => pianoSynthV2Controller.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => pianoSynthV2Controller.stopNote(note)
        }
    }
}
