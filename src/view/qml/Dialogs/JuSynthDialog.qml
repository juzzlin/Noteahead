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

// One page, laid out the way the Juno's panel is: LFO, DCO, HPF, VCF, VCA, ENV and Chorus.
AnimatedDialog {
    id: root
    title: juSynthController.displayName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 900
    height: parent ? parent.height * Constants.largeDialogScale : 700

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    onAboutToShow: () => {
        juSynthController.requestSettings();
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: juSynthController
            hostDialog: root
            // Reset role: the leftmost group of the box, and like Action it leaves the dialog open
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
            juSynthController.accept();
        }
        onRejected: () => {
            juSynthController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        JuSynthDialog_Presets {
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                JuSynthDialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.separator
            }

            ScrollView {
                id: panelScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                RowLayout {
                    width: panelScrollView.availableWidth
                    spacing: 20

                    ColumnLayout {
                        Layout.preferredWidth: 1
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 10
                        JuSynthDialog_Lfo {
                        }
                        JuSynthDialog_Hpf {
                        }
                        JuSynthDialog_Chorus {
                        }
                    }
                    JuSynthDialog_Dco {
                        Layout.preferredWidth: 1
                    }
                    JuSynthDialog_Vcf {
                        Layout.preferredWidth: 1
                    }
                    ColumnLayout {
                        Layout.preferredWidth: 1
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 10
                        JuSynthDialog_Vca {
                        }
                        JuSynthDialog_Env {
                        }
                    }
                }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => juSynthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => juSynthController.stopNote(note)
        }
    }
}
