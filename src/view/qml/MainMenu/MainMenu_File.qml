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
    title: qsTr("&File")
    Action {
        text: qsTr("New...")
        shortcut: "Ctrl+N"
        onTriggered: applicationService.requestNewProject()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Open...")
        shortcut: "Ctrl+O"
        onTriggered: applicationService.requestOpenProject()
    }
    Action {
        text: qsTr("Recent files...")
        onTriggered: UiService.requestRecentFilesDialog()
    }
    Action {
        text: qsTr("Open example song")
        onTriggered: applicationService.requestOpenExample()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Save")
        shortcut: "Ctrl+S"
        onTriggered: applicationService.requestSaveProject()
        enabled: editorService.canBeSaved
    }
    Action {
        text: qsTr("Save as...")
        shortcut: "Ctrl+A"
        onTriggered: applicationService.requestSaveProjectAs()
    }
    Action {
        text: qsTr("Save as a template...")
        onTriggered: applicationService.requestSaveProjectAsTemplate()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Export MIDI file...")
        shortcut: "Ctrl+E"
        onTriggered: applicationService.requestMidiExportDialog()
    }
    Action {
        text: qsTr("Import MIDI file...")
        shortcut: "Ctrl+I"
        onTriggered: applicationService.requestMidiImportDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Render audio...")
        shortcut: "Ctrl+R"
        onTriggered: applicationService.requestAudioRenderDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Quit")
        shortcut: "Ctrl+Q"
        onTriggered: UiService.requestQuit()
    }
    delegate: MenuItemDelegate {}
}
