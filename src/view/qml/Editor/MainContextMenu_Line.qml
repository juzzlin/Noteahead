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
    title: qsTr("Line")
    width: rootItem.width
    Action {
        text: qsTr("Insert an event...")
        shortcut: "Alt+E"
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEventSelectionDialog()
    }
    Action {
        text: qsTr("Remove current event")
        shortcut: "Alt+Shift+E"
        enabled: !UiService.isPlaying()
        onTriggered: editorService.requestEventRemoval()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Set delay...")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestLineDelayDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add MIDI CC automation...")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestLineAddMidiCcAutomationDialog()
    }
    Action {
        text: qsTr("Edit MIDI CC automations...")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialogByLine()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Add Pitch Bend automation...")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestLineAddPitchBendAutomationDialog()
    }
    Action {
        text: qsTr("Edit Pitch Bend automations...")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialogByLine()
    }
    delegate: MenuItemDelegate {}
}
