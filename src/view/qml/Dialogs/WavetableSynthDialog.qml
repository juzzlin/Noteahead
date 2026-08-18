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
    title: applicationService.wavetableSynthDeviceName
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
        wavetableSynthController.requestSettings();
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
            wavetableSynthController.accept();
        }
        onRejected: () => {
            wavetableSynthController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            readonly property real sidebarWidth: width * 0.18
            readonly property real synthAreaWidth: width - sidebarWidth - separator.width - 20
            readonly property real moduleWidth: (synthAreaWidth - (20 * 2) - 30) / 3

            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: mainRow.sidebarWidth
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                WavetableSynthDialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            Rectangle {
                id: separator
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
                Layout.leftMargin: 10
                Layout.rightMargin: 10
            }

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
                    WavetableSynthDialog_Tab1 {
                        moduleWidth: mainRow.moduleWidth
                    }
                    WavetableSynthDialog_Tab2 {
                        moduleWidth: mainRow.moduleWidth
                    }
                    WavetableSynthDialog_Tab3 {
                        moduleWidth: mainRow.moduleWidth
                    }
                }
                TabBar {
                    id: synthTabBar
                    Layout.fillWidth: true
                    TabButton {
                        text: qsTr("Oscillators")
                    }
                    TabButton {
                        text: qsTr("Filter / Envelopes")
                    }
                    TabButton {
                        text: qsTr("LFOs")
                    }
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => wavetableSynthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => wavetableSynthController.stopNote(note)
        }
    }
}
