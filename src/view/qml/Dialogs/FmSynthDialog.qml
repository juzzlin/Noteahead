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
    title: applicationService.fmSynthDeviceName
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
        fmSynthController.requestSettings();
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
            fmSynthController.accept();
        }
        onRejected: () => {
            fmSynthController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        FmSynthDialog_Presets {
        }

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Bound off the enclosing ColumnLayout's width rather than mainRow's own: mainRow's
            // width is itself derived from its children's preferred widths, which these properties
            // feed, so a self-reference here is what triggered Qt Quick Layouts' recursive-rearrange
            // guard -- see SynthDialog.qml, which has the identical structure.
            readonly property real sidebarWidth: parent.width * 0.18
            readonly property real synthAreaWidth: parent.width - sidebarWidth - separator.width - 20
            // Five columns on the first tab: the algorithm and the four operators.
            readonly property real moduleWidth: (synthAreaWidth - (20 * 4) - 30) / 5

            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: mainRow.sidebarWidth
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                FmSynthDialog_Global {
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
                    FmSynthDialog_Tab1 {
                        moduleWidth: mainRow.moduleWidth
                    }
                    FmSynthDialog_Tab2 {
                        // Four columns rather than five, so these get a slightly wider share.
                        moduleWidth: mainRow.moduleWidth * 5 / 4
                    }
                    FmSynthDialog_Tab3 {
                        moduleWidth: mainRow.moduleWidth * 5 / 3
                    }
                }
                TabBar {
                    id: synthTabBar
                    Layout.fillWidth: true
                    TabButton {
                        text: qsTr("Operators")
                    }
                    TabButton {
                        text: qsTr("Filter / Envelopes / Delay")
                    }
                    TabButton {
                        text: qsTr("LFOs")
                    }
                }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => fmSynthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => fmSynthController.stopNote(note)
        }
    }
}
