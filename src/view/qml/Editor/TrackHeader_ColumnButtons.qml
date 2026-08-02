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
import ".."
import "../ToolBar"

Item {
    id: rootItem
    signal columnDeletionRequested
    signal newColumnRequested
    ToolBarButtonBase {
        id: addColumnButton
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        enabled: !UiService.isPlaying()
        onClicked: {
            rootItem.newColumnRequested();
            focus = false;
        }
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space) {
                event.accepted = true;
            }
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Add a new note column")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/add_box.png");
        }
    }
    ToolBarButtonBase {
        id: removeColumnButton
        anchors.top: addColumnButton.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.height / 2
        height: width
        enabled: !UiService.isPlaying()
        onClicked: {
            rootItem.columnDeletionRequested();
            focus = false;
        }
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space) {
                event.accepted = true;
            }
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Remove the last note column")
        Component.onCompleted: {
            setScale(0.9);
            setImageSource("../Graphics/del_box.png");
        }
    }
}
