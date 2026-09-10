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
import Noteahead 1.0
import ".."

// The slot-level actions of a device -- the ones the rack row carries -- inside the device's own
// dialog, so a patch can be imported or the sends reached without closing the editor first. Drop it
// into a dialog's DialogButtonBox with the ResetRole and declare it first: the box lays its buttons
// out by role, and ResetRole is the leftmost group, so this always sits at the left end whether or
// not the dialog also has a Reset. The role is a layout hint only -- neither role closes a dialog.
AppButton {
    id: rootItem

    //! The dialog's own DeviceController. Everything is resolved from its device when the menu
    //! opens, never cached: the name is the only handle the UiService requests take.
    property var controller
    //! The dialog this button sits in. Needed only to close it when an import replaces the device.
    property var hostDialog

    property int _slot: -1
    property string _typeBeforeImport: ""

    text: qsTr("Device...")
    implicitWidth: Constants.defaultButtonWidth
    toolTipText: qsTr("Settings, effects and patch files of this device")
    onClicked: {
        // A second press on the button closes the menu again, which is what the closePolicy below
        // is for: the default one would have closed it on the press and left this to reopen it.
        if (deviceMenu.opened) {
            deviceMenu.close();
            return;
        }
        rootItem._slot = deviceRackController.slotOfDevice(rootItem.controller.deviceName());
        // open(), not popup(): popup() assigns x and y itself and would overwrite the placement
        // below with one that drops the menu off the bottom of the window.
        deviceMenu.open();
    }

    Menu {
        id: deviceMenu
        // Upwards out of the footer, which is the bottom edge of the dialog. A binding rather than
        // a popup() coordinate because the height is not known until the menu has been laid out.
        x: 0
        y: -height
        // Outside the button, not outside the menu: a press on the button itself has to reach the
        // click handler above with the menu still open, so that the click can toggle it shut.
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        // A popup does not inherit the theme of the dialog it was declared in, so it says what it
        // is itself -- otherwise the menu is the odd light-styled thing in a dark dialog.
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor
        delegate: MenuItemDelegate {}
        MenuItem {
            text: qsTr("Settings...")
            onClicked: UiService.requestDeviceSettingsDialog(rootItem.controller.deviceName())
        }
        MenuItem {
            text: qsTr("Insert FX...")
            onClicked: UiService.requestDeviceInsertEffectsDialog(rootItem.controller.deviceName())
        }
        MenuItem {
            text: qsTr("Sends...")
            onClicked: UiService.requestEffectSendsDialog(rootItem.controller.deviceName())
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Import Settings...")
            onClicked: {
                // Remembered so that the import can tell whether the device was replaced: a file of
                // another type puts a new device in the slot, and this dialog is the wrong editor
                // for it.
                rootItem._typeBeforeImport = deviceRackController.deviceType(rootItem._slot);
                UiService.requestImportDeviceSettings(rootItem._slot);
            }
        }
        MenuItem {
            text: qsTr("Export Settings...")
            onClicked: UiService.requestExportDeviceSettings(rootItem._slot, rootItem.controller.deviceName(), deviceRackController.deviceTypeName(rootItem._slot))
        }
    }

    Connections {
        target: deviceRackController
        function onDeviceSettingsImported(slotIndex: int): void {
            // Every device dialog is instantiated at startup, so a closed one would otherwise react
            // to an import made from the rack -- and reopen itself over it.
            if (!rootItem.hostDialog.visible || slotIndex !== rootItem._slot) {
                return;
            }
            if (deviceRackController.deviceType(slotIndex) === rootItem._typeBeforeImport) {
                // Same device, new values: nothing rebinds on its own, since the device object is
                // the one the dialog is already showing.
                rootItem.controller.requestSettings();
            } else {
                // A different device now sits in the slot. Hand over to its own editor, off the
                // closing dialog's exit transition.
                rootItem.hostDialog.close();
                Qt.callLater(() => deviceRackController.openDevice(slotIndex));
            }
        }
    }
}
