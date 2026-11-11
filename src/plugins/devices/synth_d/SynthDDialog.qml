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
    title: synthDController.displayName
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
        synthDController.requestSettings();
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: synthDController
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
            synthDController.accept();
        }
        onRejected: () => {
            synthDController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        SynthDDialog_Presets {
        }

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            readonly property real sidebarWidth: parent.width * 0.18
            readonly property real synthAreaWidth: parent.width - sidebarWidth - separator.width - 20
            readonly property real moduleWidth: (synthAreaWidth - (20 * 2) - 30) / 3
            // The first tab puts four modules on a row, the controllers and the three VCOs as on the
            // panel, and the modulation bus and the LFO under them
            readonly property real oscillatorModuleWidth: (synthAreaWidth - (20 * 3) - 30) / 4

            ScrollView {
                id: globalScrollView
                Layout.preferredWidth: mainRow.sidebarWidth
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                SynthDDialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            Rectangle {
                id: separator
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.separator
                Layout.leftMargin: 10
                Layout.rightMargin: 10
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                StackLayout {
                    currentIndex: synthTabBar.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.bottomMargin: 10

                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 4
                            columnSpacing: 20
                            width: parent.width - 20
                            SynthDDialog_Controllers {
                                Layout.preferredWidth: mainRow.oscillatorModuleWidth
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                            Repeater {
                                model: 3
                                SynthDDialog_Oscillator {
                                    number: index + 1
                                    Layout.preferredWidth: mainRow.oscillatorModuleWidth
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignTop
                                }
                            }
                            SynthDDialog_Modulation {
                                Layout.preferredWidth: mainRow.oscillatorModuleWidth
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                            SynthDDialog_Lfo {
                                Layout.preferredWidth: mainRow.oscillatorModuleWidth
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                        }
                    }

                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 3
                            columnSpacing: 20
                            width: parent.width - 20
                            SynthDDialog_Mixer {
                                Layout.preferredWidth: mainRow.moduleWidth * 2
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                        }
                    }

                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 3
                            columnSpacing: 20
                            width: parent.width - 20
                            SynthDDialog_Filter {
                                Layout.preferredWidth: mainRow.moduleWidth
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                            SynthDDialog_Contour {
                                loudness: false
                                Layout.preferredWidth: mainRow.moduleWidth
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                            SynthDDialog_Contour {
                                loudness: true
                                Layout.preferredWidth: mainRow.moduleWidth
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                            }
                        }
                    }

                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 3
                            columnSpacing: 20
                            width: parent.width - 20
                            SynthDDialog_Analog {
                                Layout.preferredWidth: mainRow.moduleWidth
                                Layout.maximumWidth: mainRow.moduleWidth
                                Layout.alignment: Qt.AlignTop
                            }
                        }
                    }
                }

                AppTabBar {
                    id: synthTabBar
                    Layout.fillWidth: true
                    AppTabButton {
                        text: qsTr("Oscillators")
                    }
                    AppTabButton {
                        text: qsTr("Mixer")
                    }
                    AppTabButton {
                        text: qsTr("Modifiers")
                    }
                    AppTabButton {
                        text: qsTr("Analog")
                    }
                }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => synthDController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => synthDController.stopNote(note)
        }
    }
}
