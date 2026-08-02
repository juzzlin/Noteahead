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

    // A Menu lays its items out in a ListView, which still reserves the height of an item that is
    // merely invisible. Without collapsing the height too, a track with no internal device opens
    // the menu with two items and two separators worth of dead space above "Line".
    MenuItem {
        text: qsTr("Insert FX...")
        visible: trackHasInternalDevice
        height: visible ? implicitHeight : 0
        onTriggered: UiService.requestDeviceInsertEffectsDialog(trackPortName)
    }
    MenuSeparator {
        visible: trackHasInternalDevice
        height: visible ? implicitHeight : 0
    }
    MenuItem {
        text: qsTr("Effect Sends...")
        visible: trackHasInternalDevice
        height: visible ? implicitHeight : 0
        onTriggered: UiService.requestEffectSendsDialog(trackPortName)
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
        text: qsTr("Edit MIDI CC automations (ALL)")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditMidiCcAutomationsDialog()
    }
    MenuSeparator {}
    Action {
        text: qsTr("Edit Pitch Bend automations (ALL)")
        enabled: !UiService.isPlaying()
        onTriggered: UiService.requestEditPitchBendAutomationsDialog()
    }
    delegate: MenuItemDelegate {}
}
