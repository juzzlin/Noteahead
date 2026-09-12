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
import "../Components"

AnimatedDialog {
    id: root
    modal: true
    width: Constants.smallDialogWidth
    property alias message: messageLabel.text
    property string acceptButtonText: qsTr("Ok")
    footer: DialogButtonBox {
        AppButton {
            text: root.acceptButtonText
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        AppButton {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
    // The content item itself rather than a child filling it: a filling child contributes no
    // implicit size, which left every confirmation collapsed to the height of its own buttons.
    contentItem: RowLayout {
        spacing: 20
        Image {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            Layout.alignment: Qt.AlignVCenter
            sourceSize: Qt.size(48, 48)
            source: "../Graphics/alert.png"
        }
        Label {
            id: messageLabel
            Layout.fillWidth: true
            Layout.minimumHeight: 48
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            color: "white"
        }
    }
    Component.onCompleted: visible = false
}
