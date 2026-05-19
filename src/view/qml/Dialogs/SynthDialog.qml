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

Dialog {
    id: root
    title: applicationService.synthDeviceName
    modal: true
    focus: true
    width: 1000
    height: 850

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    property bool isSaving: false

    onAboutToShow: () => {
        isSaving = false;
        synthController.requestSettings();
    }

    StringInputDialog {
        id: presetNameDialog
        onAccepted: {
            synthController.saveUserPreset(text);
            isSaving = false;
        }
        onRejected: {
            isSaving = false;
        }
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
            isSaving = false;
            synthController.accept();
        }
        onRejected: () => {
            isSaving = false;
            synthController.reject();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 10

        SynthDialog_Presets {
            isSaving: root.isSaving
            presetNameDialog: presetNameDialog
            onIsSavingChanged: root.isSaving = isSaving
        }

        RowLayout {
            id: mainRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Centralized width properties to ensure absolute consistency
            readonly property real sidebarWidth: width * 0.18
            readonly property real synthAreaWidth: width - sidebarWidth - separator.width - 20
            readonly property real moduleWidth: (synthAreaWidth - (20 * 2) - 30) / 3 // 20=spacing, 30=scroll padding

            // Fixed Sidebar: Global settings
            ColumnLayout {
                Layout.preferredWidth: mainRow.sidebarWidth
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignTop
                
                SynthDialog_Global {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                }
                Item { Layout.fillHeight: true } // Spacer
            }

            // Vertical Separator
            Rectangle {
                id: separator
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: "#333"
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

                    // Tab 1: Oscillators
                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 3
                            columnSpacing: 20
                            width: parent.width - 20
                            SynthDialog_Vco1 { Layout.preferredWidth: mainRow.moduleWidth; Layout.fillWidth: true; Layout.alignment: Qt.AlignTop }
                            SynthDialog_Vco2 { Layout.preferredWidth: mainRow.moduleWidth; Layout.fillWidth: true; Layout.alignment: Qt.AlignTop }
                            SynthDialog_Multi { Layout.preferredWidth: mainRow.moduleWidth; Layout.fillWidth: true; Layout.alignment: Qt.AlignTop }
                        }
                    }

                    // Tab 2: Filter / Envelope
                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 3
                            columnSpacing: 20
                            width: parent.width - 20
                            SynthDialog_Filter { Layout.preferredWidth: mainRow.moduleWidth; Layout.fillWidth: true; Layout.alignment: Qt.AlignTop }
                            SynthDialog_AmpEg { Layout.preferredWidth: mainRow.moduleWidth; Layout.fillWidth: true; Layout.alignment: Qt.AlignTop }
                            SynthDialog_ModEg { Layout.preferredWidth: mainRow.moduleWidth; Layout.fillWidth: true; Layout.alignment: Qt.AlignTop }
                        }
                    }

                    // Tab 3: LFO / Effects
                    ScrollView {
                        clip: true
                        GridLayout {
                            columns: 3
                            columnSpacing: 20
                            width: parent.width - 20
                            SynthDialog_Lfo { Layout.preferredWidth: mainRow.moduleWidth; Layout.fillWidth: true; Layout.alignment: Qt.AlignTop }
                            SynthDialog_Delay {
                                Layout.preferredWidth: mainRow.moduleWidth * 2 + 20
                                Layout.fillWidth: true
                                Layout.columnSpan: 2
                                Layout.alignment: Qt.AlignTop
                            }
                        }
                    }
                }

                TabBar {
                    id: synthTabBar
                    Layout.fillWidth: true
                    TabButton { text: qsTr("Oscillators") }
                    TabButton { text: qsTr("Filter / Envelope") }
                    TabButton { text: qsTr("LFO / Effects") }
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 10
            onNoteOnRequested: note => synthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => synthController.stopNote(note)
        }
    }
}
