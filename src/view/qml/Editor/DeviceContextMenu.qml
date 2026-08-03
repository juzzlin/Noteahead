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

// The actions of one internal device, offered both as a sub-menu of the editor context menu and as
// the context menu of the device button in the track header. Mirrors the Device Rack rows.
Menu {
    id: rootItem

    title: qsTr("Device")

    property string portName: ""

    // Resolved on demand rather than bound: the rack can be re-arranged while this menu exists, and
    // slotOfDevice() has no change notification to re-evaluate a binding on.
    function deviceSlot(): int {
        return deviceRackController.slotOfDevice(rootItem.portName);
    }

    MenuItem {
        text: qsTr("Open Device...")
        onTriggered: UiService.requestDeviceDialog(rootItem.portName)
    }
    MenuSeparator {}
    MenuItem {
        text: qsTr("Insert FX...")
        onTriggered: UiService.requestDeviceInsertEffectsDialog(rootItem.portName)
    }
    MenuItem {
        text: qsTr("Effect Sends...")
        onTriggered: UiService.requestEffectSendsDialog(rootItem.portName)
    }
    MenuSeparator {}
    MenuItem {
        text: qsTr("Settings...")
        onTriggered: UiService.requestDeviceSettingsDialog(rootItem.portName)
    }
    Menu {
        title: qsTr("Manage")
        width: rootItem.width
        MenuItem {
            text: qsTr("Export Settings...")
            onTriggered: {
                const slot = rootItem.deviceSlot();
                if (slot >= 0) {
                    UiService.requestExportDeviceSettings(slot, rootItem.portName, deviceRackController.deviceTypeName(slot));
                }
            }
        }
        MenuItem {
            text: qsTr("Import Settings...")
            onTriggered: {
                const slot = rootItem.deviceSlot();
                if (slot >= 0) {
                    UiService.requestImportDeviceSettings(slot);
                }
            }
        }
        delegate: MenuItemDelegate {}
    }
    delegate: MenuItemDelegate {}
}
