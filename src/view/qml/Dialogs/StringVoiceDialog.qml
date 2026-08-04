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
    title: applicationService.stringVoiceDeviceName
    modal: true
    focus: true
    width: 1400
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
        stringVoiceController.requestSettings();
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
            stringVoiceController.accept();
        }
        onRejected: () => {
            stringVoiceController.reject();
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

            // Global settings
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: globalScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StringVoiceDialog_Global {
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

            // Ensemble Chorus section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: ensembleScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StringVoiceDialog_Ensemble {
                        width: ensembleScrollView.availableWidth
                    }
                }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Strings section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: stringsScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StringVoiceDialog_Strings {
                        width: stringsScrollView.availableWidth
                    }
                }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Voice Section
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: voiceScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StringVoiceDialog_Voice {
                        width: voiceScrollView.availableWidth
                    }
                }
            }

            // Vertical Separator
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
            }

            // Balance and Vocoder
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop

                ScrollView {
                    id: balanceScrollView
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        width: balanceScrollView.availableWidth
                        spacing: 12

                        StringVoiceDialog_Balance {
                            Layout.fillWidth: true
                        }

                        LayoutSeparator {
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                        }

                        StringVoiceDialog_Vocoder {
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => stringVoiceController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => stringVoiceController.stopNote(note)
        }
    }
}
