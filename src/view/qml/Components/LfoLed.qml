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
//
// Every layer is painted once; only their opacities follow the LFO. The halo spills over the
// neighbours like the light of a real LED does, but takes no room in the layout.
Item {
    id: root

    property var controller: null
    property int lfoIndex: 0
    //! 0 at the bottom of the shape, 1 at the top.
    property real level: 0.0

    // Above the neighbours, for the halo to light them rather than hide under them
    z: 1
    implicitWidth: 14
    implicitHeight: 14

    readonly property real lensRadius: width / 2 - 1.5

    Canvas {
        id: halo
        anchors.centerIn: parent
        width: root.width * 2.4
        height: root.height * 2.4
        opacity: root.level * root.level
        onPaint: {
            const context = getContext("2d");
            context.reset();
            const centre = width / 2;
            const gradient = context.createRadialGradient(centre, centre, root.lensRadius * 0.5, centre, centre, centre);
            gradient.addColorStop(0.0, Qt.rgba(1.0, 0.25, 0.15, 0.35));
            gradient.addColorStop(0.35, Qt.rgba(1.0, 0.12, 0.06, 0.12));
            gradient.addColorStop(1.0, Qt.rgba(1.0, 0.0, 0.0, 0.0));
            context.fillStyle = gradient;
            context.fillRect(0, 0, width, height);
        }
    }

    // The bezel and the unlit lens: a dark red dome, a little lighter at the top where it catches
    // the room light
    Canvas {
        id: unlit
        anchors.fill: parent
        onPaint: {
            const context = getContext("2d");
            context.reset();
            const centre = width / 2;
            context.beginPath();
            context.arc(centre, centre, width / 2 - 0.5, 0, 2 * Math.PI);
            context.fillStyle = "#1a0a0a";
            context.fill();
            const gradient = context.createRadialGradient(centre - root.lensRadius * 0.3, centre - root.lensRadius * 0.4, 0, centre, centre, root.lensRadius);
            gradient.addColorStop(0.0, "#6a2020");
            gradient.addColorStop(1.0, "#2a0808");
            context.beginPath();
            context.arc(centre, centre, root.lensRadius, 0, 2 * Math.PI);
            context.fillStyle = gradient;
            context.fill();
        }
    }

    // The lit lens: a hot, almost white core fading to deep red at the rim
    Canvas {
        id: lit
        anchors.fill: parent
        opacity: root.level
        onPaint: {
            const context = getContext("2d");
            context.reset();
            const centre = width / 2;
            const gradient = context.createRadialGradient(centre, centre, 0, centre, centre, root.lensRadius);
            gradient.addColorStop(0.0, "#fff0e0");
            gradient.addColorStop(0.3, "#ff5a40");
            gradient.addColorStop(0.75, "#e01010");
            gradient.addColorStop(1.0, "#900000");
            context.beginPath();
            context.arc(centre, centre, root.lensRadius, 0, 2 * Math.PI);
            context.fillStyle = gradient;
            context.fill();
        }
    }

    // The reflection on the dome, there lit or not
    Canvas {
        anchors.fill: parent
        onPaint: {
            const context = getContext("2d");
            context.reset();
            const x = width / 2 - root.lensRadius * 0.35;
            const y = height / 2 - root.lensRadius * 0.4;
            const radius = root.lensRadius * 0.45;
            const gradient = context.createRadialGradient(x, y, 0, x, y, radius);
            gradient.addColorStop(0.0, Qt.rgba(1.0, 1.0, 1.0, 0.6));
            gradient.addColorStop(1.0, Qt.rgba(1.0, 1.0, 1.0, 0.0));
            context.fillStyle = gradient;
            context.fillRect(0, 0, width, height);
        }
    }

    Timer {
        interval: 33 // ~30 FPS
        repeat: true
        running: root.visible && root.controller !== null
        onTriggered: root.level = (root.controller.lfoLed(root.lfoIndex) + 1.0) / 2.0
    }
}
