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

// LFO phase LED. Glows with the LFO's shape: bright at the top of the cycle, dark at the bottom, so
// its rate and waveform can be read at a glance. Reads the device only while it is on screen.
Rectangle {
    id: root

    property var controller: null
    property int lfoIndex: 0
    //! 0 at the bottom of the shape, 1 at the top.
    property real level: 0.0

    implicitWidth: 12
    implicitHeight: 12

    radius: width / 2
    color: Qt.rgba(0.23 + 0.77 * root.level, 0.06 + 0.07 * root.level, 0.06 + 0.07 * root.level, 1.0)
    border.color: "#552020"
    border.width: 1

    Timer {
        interval: 33 // ~30 FPS
        repeat: true
        running: root.visible && root.controller !== null
        onTriggered: root.level = (root.controller.lfoLed(root.lfoIndex) + 1.0) / 2.0
    }
}
