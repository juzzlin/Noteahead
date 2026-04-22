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

    // In the Device sub-menu's place on a track that has none, so that the first thing the menu
    // offers is always about the track's device. Collapsed by hand: a menu's ListView still reserves
    // the height of a merely invisible row.
    MenuItem {
        text: qsTr("Assign a device...")
        visible: !trackHasInternalDevice
        height: visible ? implicitHeight : 0
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestAssignDevice(editorService.position.track)
    }
    // MenuItemDelegate hides and collapses the row of a disabled sub-menu, which is what keeps a
    // track with no internal device from opening the menu with dead space above "Line".
    DeviceContextMenu {
        portName: trackPortName
        enabled: trackHasInternalDevice
        width: rootItem.width
    }
    MenuSeparator {}
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
    // In a sub-menu of their own, one or two entries for each kind, to keep the menu short
    Menu {
        title: qsTr("Automation")
        width: rootItem.width
        // The adds go on the column the cursor is in, over the whole pattern, as from the Column
        // menu. Only the parameter automation's is wider: its target list holds every device and
        // effect, the master and the sends included, rather than only the track's own device.
        Action {
            text: qsTr("Add MIDI CC automation...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestColumnAddMidiCcAutomationDialog()
        }
        Action {
            text: qsTr("Edit MIDI CC automations (whole song)...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestEditMidiCcAutomationsDialog()
        }
        MenuSeparator {}
        Action {
            text: qsTr("Add Pitch Bend automation...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestColumnAddPitchBendAutomationDialog()
        }
        Action {
            text: qsTr("Edit Pitch Bend automations (whole song)...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestEditPitchBendAutomationsDialog()
        }
        MenuSeparator {}
        Action {
            text: qsTr("Add parameter automation (all targets)...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestAddParameterAutomationDialog()
        }
        Action {
            text: qsTr("Edit parameter automations (whole song)...")
            enabled: !UiService.isPlaying()
            onTriggered: UiService.requestEditParameterAutomationsDialog()
        }
        delegate: MenuItemDelegate {}
    }
    delegate: MenuItemDelegate {}
}
