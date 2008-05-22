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
    title: juSynth08Controller.displayName
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 1000
    height: parent ? parent.height * Constants.largeDialogScale : 850

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    onAboutToShow: () => {
        juSynth08Controller.requestSettings();
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: juSynth08Controller
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
            juSynth08Controller.accept();
        }
        onRejected: () => {
            juSynth08Controller.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        JuSynth08Dialog_Presets {
        }

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            readonly property real sidebarWidth: parent.width * 0.18
            readonly property real synthAreaWidth: parent.width - sidebarWidth - separator.width - 20
            readonly property real moduleWidth: (synthAreaWidth - (20 * 2) - 30) / 3
            // The oscillator tab puts four modules on one row, LFO to VCO 2 as on the Jupiter-8's
            // panel, so it divides the same width by four. See SynthDialog.qml.
            readonly property real vcoModuleWidth: (synthAreaWidth - (20 * 3) - 30) / 4

            // Fixed Sidebar: Global settings
            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: mainRow.sidebarWidth
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                JuSynth08Dialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            // Vertical Separator
            Rectangle {
                id: separator
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.separator
                Layout.leftMargin: 10
                Layout.rightMargin: 10
            }

            // Tabbed Synthesis Area
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                StackLayout {
                    id: synthStackLayout
                    currentIndex: synthTabBar.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.bottomMargin: 10

                    JuSynth08Dialog_Tab1 {
                        moduleWidth: mainRow.vcoModuleWidth
                    }

                    JuSynth08Dialog_Tab2 {
                        moduleWidth: mainRow.moduleWidth
                    }

                    JuSynth08Dialog_Tab3 {
                        moduleWidth: mainRow.moduleWidth
                    }
                }

                AppTabBar {
                    id: synthTabBar
                    Layout.fillWidth: true
                    AppTabButton {
                        text: qsTr("Oscillators")
                    }
                    AppTabButton {
                        text: qsTr("Filter")
                    }
                    AppTabButton {
                        text: qsTr("Amp / Delay")
                    }
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => juSynth08Controller.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => juSynth08Controller.stopNote(note)
        }
    }
}
