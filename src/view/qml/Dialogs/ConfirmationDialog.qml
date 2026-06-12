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
    modal: true
    property alias message: messageLabel.text
    property string acceptButtonText: qsTr("Ok")
    footer: DialogButtonBox {
        Button {
            text: root.acceptButtonText
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            text: qsTr("Cancel")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        Image {
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48
            sourceSize: Qt.size(48, 48)
            source: "../Graphics/alert.png"
        }
        Label {
            id: messageLabel
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: "white"
        }
    }
    Component.onCompleted: visible = false
}
