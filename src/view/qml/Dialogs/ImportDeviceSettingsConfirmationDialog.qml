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

Dialog {
    id: root
    title: qsTr("Device type mismatch")
    modal: true
    focus: true

    property int slotIndex: -1
    property url fileUrl
    property string currentTypeName
    property string importedTypeName

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    Label {
        text: qsTr("The file contains settings for '%1' but the current device is '%2'. Replace it anyway?")
            .arg(root.importedTypeName)
            .arg(root.currentTypeName)
        wrapMode: Text.WordWrap
        width: 400
        color: "white"
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Replace")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                deviceRackController.confirmImportSettings(root.slotIndex, root.fileUrl);
                root.close();
            }
        }
        Button {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: root.close()
        }
    }
}
