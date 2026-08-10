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
    title: qsTr("&Song")
    // The sequences of these actions are registered as application shortcuts in Main.qml so that they
    // also work when a modal dialog is open. See ShortcutHintAction.
    ShortcutHintAction {
        text: qsTr("Play")
        shortcutHint: "F5"
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestPlay()
    }
    ShortcutHintAction {
        text: qsTr("Stop")
        shortcutHint: "F6"
        enabled: UiService.isPlaying()
        onTriggered: UiService.requestStop()
    }
    ShortcutHintAction {
        text: qsTr("Rewind")
        shortcutHint: "F8"
        enabled: !UiService.isPlaying()
        onTriggered: UiService.rewindSong()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Settings...")
        onTriggered: UiService.requestSongSettingsDialog()
    }
    delegate: MenuItemDelegate {}
}
