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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

// Two pages: the sound and what shapes it, then the sweeps, the LFO and the gate that move it.
AnimatedDialog {
    id: root
    title: noiseFxController.displayName
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
        noiseFxController.requestSettings();
    }

    footer: DialogButtonBox {
        DeviceMenuButton {
            controller: noiseFxController
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
            noiseFxController.accept();
        }
        onRejected: () => {
            noiseFxController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        PresetRow {
            presetNames: noiseFxController.presetNames
            currentIndex: noiseFxController.currentPresetIndex
            currentIsUserPreset: noiseFxController.currentPresetIsUserPreset
            presetTarget: noiseFxController
            onPresetActivated: index => noiseFxController.loadPreset(index)
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

                NoiseFxDialog_Global {
                    width: globalScrollView.availableWidth
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.separator
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                StackLayout {
                    currentIndex: tabBar.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollView {
                        id: soundScrollView
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        RowLayout {
                            width: soundScrollView.availableWidth
                            spacing: 20
                            NoiseFxDialog_Noise {
                                Layout.preferredWidth: 1
                            }
                            NoiseFxDialog_Tone {
                                Layout.preferredWidth: 1
                            }
                            NoiseFxDialog_Filter {
                                Layout.preferredWidth: 1
                            }
                            NoiseFxDialog_Amp {
                                Layout.preferredWidth: 1
                            }
                        }
                    }

                    ScrollView {
                        id: modulationScrollView
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        RowLayout {
                            width: modulationScrollView.availableWidth
                            spacing: 20
                            Repeater {
                                model: noiseFxController.sweeps
                                NoiseFxDialog_Sweep {
                                    required property var modelData
                                    sweep: modelData
                                    Layout.preferredWidth: 1
                                }
                            }
                            NoiseFxDialog_Lfo {
                                Layout.preferredWidth: 1
                            }
                            NoiseFxDialog_Gate {
                                Layout.preferredWidth: 1
                            }
                        }
                    }
                }

                AppTabBar {
                    id: tabBar
                    Layout.fillWidth: true
                    AppTabButton {
                        text: qsTr("Sound")
                    }
                    AppTabButton {
                        text: qsTr("Modulation")
                    }
                }
            }
        }

        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => noiseFxController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => noiseFxController.stopNote(note)
        }
    }
}
