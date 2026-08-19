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

MenuItem {
    id: rootItem
    // A sub-menu's own visible property drives its popup, not this row, so a menu can only hide a
    // whole sub-menu by disabling it. A Menu lays its items out in a ListView, which still reserves
    // the height of a merely invisible item, hence the collapse.
    visible: !subMenu || subMenu.enabled
    height: visible ? implicitHeight : 0
    contentItem: Item {
        anchors.centerIn: parent
        Text {
            text: rootItem.text
            anchors.left: parent.left
            color: "white"
        }
        Text {
            text: rootItem?.action?.shortcut || rootItem?.action?.shortcutHint || ""
            anchors.right: parent.right
            color: "white"
        }
    }
    // Same fix as AppButton: MenuItem's own click detection can drop clicks while the audio engine
    // is running, so a plain MouseArea takes over. A submenu item still has to open on hover/click
    // through the built-in handling instead, so it is left alone here. Re-emitting clicked() alone
    // is not enough here: an Action-backed item only runs its onTriggered through action.trigger(),
    // a plain item is wired to either onClicked or onTriggered depending on the menu, and the menu
    // isn't guaranteed to close itself off a synthetic signal -- so all of it is done by hand.
    MouseArea {
        anchors.fill: parent
        enabled: !rootItem.subMenu
        onClicked: {
            if (rootItem.action) {
                rootItem.action.trigger(rootItem);
            } else {
                rootItem.clicked();
                rootItem.triggered();
            }
            if (rootItem.menu) {
                rootItem.menu.close();
            }
        }
    }
}
