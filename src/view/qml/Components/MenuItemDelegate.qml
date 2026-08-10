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
}
