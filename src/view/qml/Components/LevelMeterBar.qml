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

// Horizontal peak/RMS meter with a gain-staging target marker. The bar shows RMS, the thin line
// shows peak, and the marker is where a gain staged device should be sitting.
Item {
    id: root

    property real peakDb: -120
    property real rmsDb: -120
    property real minimumDb: -60
    property real maximumDb: 0
    property real markerDb: -18

    implicitHeight: 18

    function positionOf(db) {
        const clamped = Math.max(root.minimumDb, Math.min(root.maximumDb, db));
        return (clamped - root.minimumDb) / (root.maximumDb - root.minimumDb);
    }

    Rectangle {
        id: track
        anchors.fill: parent
        color: "#1a1a1a"
        border.color: "#555"
        radius: 2

        Rectangle {
            id: rmsBar
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            width: Math.max(0, (track.width - 2) * root.positionOf(root.rmsDb))
            radius: 1
            // Green up to the target, amber above it, red once it is close to clipping.
            color: root.peakDb > -1 ? "#d04040" : (root.rmsDb > root.markerDb ? "#d0a040" : "#40a060")
        }

        Rectangle {
            id: peakLine
            width: 2
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            x: Math.max(1, Math.min(track.width - 3, (track.width - 2) * root.positionOf(root.peakDb)))
            color: root.peakDb > -1 ? "#ff6060" : "#e0e0e0"
            visible: root.peakDb > root.minimumDb
        }

        Rectangle {
            width: 1
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            x: (track.width - 2) * root.positionOf(root.markerDb) + 1
            color: "#8899cc"
            opacity: 0.9
        }
    }

    ToolTip.visible: hoverArea.containsMouse
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.text: qsTr("Pre-insert level: %1 dBFS peak, %2 dBFS RMS. The marker is %3 dBFS.").arg(root.peakDb <= root.minimumDb ? "-∞" : root.peakDb.toFixed(1)).arg(root.rmsDb <= root.minimumDb ? "-∞" : root.rmsDb.toFixed(1)).arg(root.markerDb)

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
