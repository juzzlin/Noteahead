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
    id: rootItem
    title: qsTr("&File")
    // Anything that replaces the current song is disabled while playing. The player worker runs in
    // its own thread against the events and instruments of the song being replaced, so swapping it
    // out mid-playback leaves notes hanging on devices that have already been reset. Saving and
    // exporting stay available: they read the song rather than replace it.
    readonly property bool _canReplaceSong: !UiService.isPlaying()
    Action {
        text: qsTr("New...")
        shortcut: "Ctrl+N"
        enabled: rootItem._canReplaceSong
        onTriggered: applicationService.requestNewProject()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Open...")
        shortcut: "Ctrl+O"
        enabled: rootItem._canReplaceSong
        onTriggered: applicationService.requestOpenProject()
    }
    Action {
        text: qsTr("Recent files...")
        enabled: rootItem._canReplaceSong
        onTriggered: UiService.requestRecentFilesDialog()
    }
    MenuSeparator {}
    Menu {
        title: qsTr("Examples")
        enabled: rootItem._canReplaceSong
        Action {
            text: qsTr("Example Song 1")
            enabled: rootItem._canReplaceSong
            onTriggered: applicationService.requestOpenExample()
        }
        delegate: MenuItemDelegate {}
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
        enabled: rootItem._canReplaceSong
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
