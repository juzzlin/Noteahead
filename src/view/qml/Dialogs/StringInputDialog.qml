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
import QtQuick.Dialogs
import QtQuick.Layouts

AnimatedDialog {
    id: rootItem
    modal: true
    anchors.centerIn: parent
    width: parent.width * 0.4
    standardButtons: Dialog.Ok | Dialog.Cancel
    property alias text: textField.text
    function setTitle(titleText) {
        title = "<strong>" + titleText + "</strong>";
    }
    contentItem: RowLayout {
        spacing: 10
        width: parent.width
        Label {
            text: qsTr("Name:")
        }
        TextField {
            id: textField
            width: parent.width * 0.8
            focus: true
            selectByMouse: true
            Keys.onReturnPressed: {
                focus = false;
                rootItem.accept();
            }
            Layout.fillWidth: true
        }
    }
    onOpened: textField.forceActiveFocus()
}
