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
    title: pianoSynthAcousticController.displayName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 920
    height: parent ? parent.height * Constants.largeDialogScale : 680

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    onAboutToShow: () => {
        pianoSynthAcousticController.requestSettings();
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: pianoSynthAcousticController
            hostDialog: root
            DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
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
            pianoSynthAcousticController.accept();
        }
        onRejected: () => {
            pianoSynthAcousticController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 12

        // Presets header
        PianoSynthAcousticDialog_Presets {
            Layout.fillWidth: true
        }

        // Horizontal separator under presets
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.separator
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 15

            // Global sidebar
            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: 180
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                PianoSynthAcousticDialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            // Vertical separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.separator
            }

            // Hammer section
            ScrollView {
                id: hammerScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                PianoSynthAcousticDialog_Hammer {
                    width: hammerScrollView.availableWidth
                }
            }

            // Vertical separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.separator
            }

            // String section
            ScrollView {
                id: stringScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                PianoSynthAcousticDialog_String {
                    width: stringScrollView.availableWidth
                }
            }

            // Vertical separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.separator
            }

            // Dampers section
            ScrollView {
                id: dampersScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                PianoSynthAcousticDialog_Dampers {
                    width: dampersScrollView.availableWidth
                }
            }
        }

        // Virtual keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 8
            onNoteOnRequested: note => pianoSynthAcousticController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => pianoSynthAcousticController.stopNote(note)
        }
    }
}
