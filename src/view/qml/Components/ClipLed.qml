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
import Noteahead 1.0

// Clip LED. Latches on any full-scale sample in the device's output and stays lit until clicked, so
// a single overshoot cannot go unnoticed. The latch itself lives in the device; this only shows it
// and asks for it to be cleared.
Rectangle {
    id: root

    property bool clipped: false

    signal clicked

    implicitWidth: 14
    implicitHeight: 14

    radius: width / 2
    color: root.clipped ? "#ff2020" : "#3a1010"
    border.color: root.clipped ? "#ff8080" : "#552020"
    border.width: 1

    ToolTip.visible: mouseArea.containsMouse
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.text: root.clipped ? qsTr("Output clipped. Click to clear.") : qsTr("No clipping")

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
