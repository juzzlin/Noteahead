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
    id: rootItem

    readonly property string trackPortName: editorService.instrumentPortName(editorService.position.track)
    readonly property bool trackHasInternalDevice: deviceService.isInternalDevice(trackPortName)

    // MenuItemDelegate hides and collapses the row of a disabled sub-menu, which is what keeps a
    // track with no internal device from opening the menu with dead space above "Line".
    DeviceContextMenu {
        portName: trackPortName
        enabled: trackHasInternalDevice
        width: rootItem.width
    }
    MenuSeparator {
        visible: trackHasInternalDevice
        height: visible ? implicitHeight : 0
    }
    MainContextMenu_Line {}
    MenuSeparator {}
    MainContextMenu_Column {}
    MenuSeparator {}
    MainContextMenu_Track {}
    MenuSeparator {}
    MainContextMenu_Pattern {}
    MenuSeparator {}
    MainContextMenu_Selection {}
    MenuSeparator {}
    MainContextMenu_Song {}
    MenuSeparator {}
    Action {
        text: qsTr("Edit MIDI CC automations (ALL)...")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Edit Pitch Bend automations (ALL)...")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialog()
    }
    delegate: MenuItemDelegate {}
}
