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
    title: applicationService.speechDeviceName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    onAboutToShow: () => {
        speechController.requestSettings();
    }

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Reset")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Restore every control to its factory default, the phrase included.")
            onClicked: speechController.reset()
        }
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
            speechController.accept();
        }
        onRejected: () => {
            speechController.reject();
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

            // Global settings sit in a fixed sidebar on the left, as on every other device dialog.
            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: 200
                Layout.minimumWidth: 150
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                SpeechDialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            ScrollView {
                id: phraseScrollView
                // The phrase field is this device's main control, so it takes the largest share of
                // what is left over. The minimums are what keep all four columns on screen at the
                // smallest window the application runs at.
                Layout.fillWidth: true
                Layout.preferredWidth: 320
                Layout.minimumWidth: 200
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                SpeechDialog_Phrase {
                    width: phraseScrollView.availableWidth
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            ScrollView {
                id: timingScrollView
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                Layout.minimumWidth: 150
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                SpeechDialog_Timing {
                    width: timingScrollView.availableWidth
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            ScrollView {
                id: voiceScrollView
                Layout.fillWidth: true
                Layout.preferredWidth: 200
                Layout.minimumWidth: 150
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                SpeechDialog_Voice {
                    width: voiceScrollView.availableWidth
                }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => speechController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => speechController.stopNote(note)
        }
    }
}
