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
    title: applicationService.kick808DeviceName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 640

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    onAboutToShow: () => {
        kick808Controller.requestSettings();
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        onAccepted: () => {
            kick808Controller.accept();
        }
        onRejected: () => {
            kick808Controller.reject();
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

            // Fixed sidebar: global settings
            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Kick808Dialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            // Vertical separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Kick section
            ScrollView {
                id: kickScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Kick808Dialog_Kick {
                    width: kickScrollView.availableWidth
                }
            }

            // Vertical separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Pitch section
            ScrollView {
                id: pitchScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Kick808Dialog_Pitch {
                    width: pitchScrollView.availableWidth
                }
            }
        }

        // Virtual keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => kick808Controller.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => kick808Controller.stopNote(note)
        }
    }
}
