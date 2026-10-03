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
    title: qsTr("Pattern")
    width: rootItem.width
    Action {
        text: qsTr("Cut")
        shortcut: "Ctrl+F3"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestPatternCut()
    }
    Action {
        text: qsTr("Copy")
        shortcut: "Ctrl+F4"
        onTriggered: editorService.requestPatternCopy()
    }
    Action {
        text: qsTr("Paste")
        shortcut: "Ctrl+F5"
        enabled: !UiService.isPlaying() && editorService.hasPatternToPaste
        onTriggered: editorService.requestPatternPaste()
    }
    MenuSeparator {}
    // In a sub-menu of its own: eight steps each way, and the all-patterns ones under that, made the
    // menu taller than a small window. The shortcuts still work, as Qt finds the window of an action
    // in a sub-menu through its parent menus.
    Menu {
        title: qsTr("Transpose")
        width: rootItem.width
        Action {
            text: qsTr("Transpose <b>+1</b> semitones")
            shortcut: "Ctrl+F10"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(1)
        }
        Action {
            text: qsTr("Transpose <b>-1</b> semitones")
            shortcut: "Ctrl+F9"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(-1)
        }
        Action {
            text: qsTr("Transpose <b>+2</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(2)
        }
        Action {
            text: qsTr("Transpose <b>-2</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(-2)
        }
        Action {
            text: qsTr("Transpose <b>+6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(6)
        }
        Action {
            text: qsTr("Transpose <b>-6</b> semitones")
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(-6)
        }
        Action {
            text: qsTr("Transpose <b>+12</b> semitones")
            shortcut: "Ctrl+F12"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(12)
        }
        Action {
            text: qsTr("Transpose <b>-12</b> semitones")
            shortcut: "Ctrl+F11"
            enabled: !UiService.isPlaying()
            onTriggered: editorService.requestPatternTranspose(-12)
        }
        delegate: MenuItemDelegate {}
    }
    // In a sub-menu of their own, one or two entries for each kind, to keep the menu short
    Menu {
        title: qsTr("Automation")
        width: rootItem.width
        Action {
            text: qsTr("Edit MIDI CC automations...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestEditMidiCcAutomationsDialogByPattern()
        }
        MenuSeparator {}
        Action {
            text: qsTr("Edit Pitch Bend automations...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestEditPitchBendAutomationsDialogByPattern()
        }
        MenuSeparator {}
        Action {
            text: qsTr("Edit parameter automations...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestEditParameterAutomationsDialogByPattern()
        }
        delegate: MenuItemDelegate {}
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add Note OFF")
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestNoteOffAtPatternFirstLine()
    }
    delegate: MenuItemDelegate {}
}
