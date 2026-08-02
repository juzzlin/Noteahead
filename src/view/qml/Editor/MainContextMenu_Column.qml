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

import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."
import "../Components"

Menu {
    title: qsTr("Column")
    width: rootItem.width
    Action {
        text: qsTr("Settings")
        onTriggered: UiService.requestColumnSettingsDialog(editorService.position.track, editorService.position.column)
    }
    MenuSeparator {}
    Action {
        text: qsTr("Cut")
        shortcut: "Alt+F3"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnCut()
    }
    Action {
        text: qsTr("Copy")
        shortcut: "Alt+F4"
        onTriggered: editorService.requestColumnCopy()
    }
    Action {
        text: qsTr("Paste")
        shortcut: "Alt+F5"
        enabled: !UiService.isPlaying() && editorService.hasColumnToPaste
        onTriggered: editorService.requestColumnPaste()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Transpose <b>+1</b> semitones")
        shortcut: "Alt+F10"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(1)
    }
    Action {
        text: qsTr("Transpose <b>-1</b> semitones")
        shortcut: "Alt+F9"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(-1)
    }
    Action {
        text: qsTr("Transpose <b>+2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(2)
    }
    Action {
        text: qsTr("Transpose <b>-2</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(-2)
    }
    Action {
        text: qsTr("Transpose <b>+6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(6)
    }
    Action {
        text: qsTr("Transpose <b>-6</b> semitones")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(-6)
    }
    Action {
        text: qsTr("Transpose <b>+12</b> semitones")
        shortcut: "Alt+F12"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(12)
    }
    Action {
        text: qsTr("Transpose <b>-12</b> semitones")
        shortcut: "Alt+F11"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestColumnTranspose(-12)
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add Note OFF")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestNoteOffAtColumnFirstLine()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Interpolate velocity")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnVelocityInterpolationDialog()
    }
    Action {
        text: qsTr("Interpolate pan")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnPanInterpolationDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add MIDI CC automation")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnAddMidiCcAutomationDialog()
    }
    Action {
        text: qsTr("Edit MIDI CC automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialogByColumn()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add Pitch Bend automation")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestColumnAddPitchBendAutomationDialog()
    }
    Action {
        text: qsTr("Edit Pitch Bend automations")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialogByColumn()
    }
    delegate: MenuItemDelegate {}
}
