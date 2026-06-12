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
import QtQuick.Layouts 1.15
import Noteahead 1.0

Dialog {
    id: root
    title: qsTr("Effect type mismatch")
    modal: true
    focus: true

    property int slotIndex: -1
    property url fileUrl
    property string currentType
    property string importedType

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20

        Label {
            text: qsTr("The file contains settings for '%1' but the current effect is '%2'. Replace it anyway?")
                .arg(root.importedType)
                .arg(root.currentType)
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.preferredWidth: 400
            color: "white"
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Replace")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                effectRackController.confirmImportEffectSettings(root.slotIndex, root.fileUrl);
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
