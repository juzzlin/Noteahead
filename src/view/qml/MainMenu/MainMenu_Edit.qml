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
import ".."
import "../Components"

Menu {
    title: qsTr("&Edit")
    Action {
        text: qsTr("Toggle edit mode")
        shortcut: "Esc"
        onTriggered: applicationService.toggleEditMode()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Undo")
        shortcut: "Ctrl+Z"
        enabled: editorService.canUndo
        onTriggered: editorService.undo()
    }
    Action {
        text: qsTr("Redo")
        shortcut: "Ctrl+Y"
        enabled: editorService.canRedo
        onTriggered: editorService.redo()
    }
    Action {
        text: qsTr("Delete unused patterns")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestDeleteUnusedPatterns()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Reset instruments")
        onTriggered: applicationService.requestInstrumentReset()
    }
    Action {
        text: qsTr("Stop all notes")
        onTriggered: applicationService.requestAllNotesOff()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Settings...")
        onTriggered: UiService.requestSettingsDialog()
    }
    delegate: MenuItemDelegate {}
}
