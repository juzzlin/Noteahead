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

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GridLayout {
                columns: 5
                columnSpacing: 20
                rowSpacing: 10
                width: parent.width - 20

                SynthDialog_Global {}
                SynthDialog_Vco1 {}
                SynthDialog_Vco2 {}
                SynthDialog_Multi {}
                SynthDialog_Filter {}

                // Spacing Row
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    Layout.columnSpan: 5
                }

                SynthDialog_Lfo {}
                SynthDialog_AmpEg {}
                SynthDialog_ModEg {}
                SynthDialog_Delay {}
                Item {
                    Layout.fillWidth: true
                }
            }
        }

        // Virtual Keyboard
        VirtualKeyboard {
            Layout.fillWidth: true
            Layout.topMargin: 20
            onNoteOnRequested: note => synthController.playNote(note, UiService._activeVelocity / 127.0)
            onNoteOffRequested: note => synthController.stopNote(note)
        }
    }
}
