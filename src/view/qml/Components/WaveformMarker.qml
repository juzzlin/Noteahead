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

//! A handle on a waveform: a vertical line with a tab at the top, dragged along the item it sits in.
//! It reports where it was dragged to and lets its owner decide what that means, so a marker whose
//! owner refuses the move simply does not follow the mouse.
Item {
    id: root

    //! Where the marker sits, as a fraction of the width of the item it is placed in.
    property double position: 0
    //! How far the marker can be dragged, as fractions of that same width. The neighbouring markers
    //! are what these are usually set to: a start that can pass its end trims the pad to nothing.
    property double minimumPosition: 0
    property double maximumPosition: 1
    //! White: the pictures these sit on are drawn in the accent colour, which a marker has to stand
    //! out from rather than join.
    readonly property color markerColor: "white"
    //! Dashed, for a marker that would otherwise be just another plain vertical on the picture.
    property bool dashed: false
    //! Which way the tab points. One for a marker whose own material lies to its right, minus one for
    //! a marker that closes a stretch from the other side.
    property int direction: 1

    signal moved(double newPosition)

    //! Wider than the line it draws, so that the marker can be taken hold of without hitting a pixel.
    readonly property int grabWidth: 18
    readonly property int tabSize: 12
    readonly property int lineX: Math.floor(grabWidth / 2)

    width: grabWidth
    // Clamped: a track anchored with margins inside an item that has not been laid out yet is
    // negative in height, and the dashes count themselves off it.
    height: parent ? Math.max(0, parent.height) : 0
    x: root.position * (parent ? parent.width : 0) - lineX
    opacity: mouseArea.containsMouse || mouseArea.pressed ? 1.0 : 0.9

    Column {
        x: root.lineX
        spacing: root.dashed ? 4 : 0
        Repeater {
            model: root.dashed ? Math.ceil(root.height / 8) : 1
            Rectangle {
                width: 1
                height: root.dashed ? 4 : root.height
                color: root.markerColor
            }
        }
    }

    Canvas {
        id: tab
        x: root.direction > 0 ? root.lineX : root.lineX - width
        width: root.tabSize
        height: root.tabSize

        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.fillStyle = root.markerColor;
            ctx.beginPath();
            if (root.direction > 0) {
                ctx.moveTo(0, 0);
                ctx.lineTo(width, 0);
                ctx.lineTo(0, height);
            } else {
                ctx.moveTo(width, 0);
                ctx.lineTo(0, 0);
                ctx.lineTo(width, height);
            }
            ctx.closePath();
            ctx.fill();
        }
    }

    onDirectionChanged: tab.requestPaint()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor

        //! Where the marker was taken hold of, relative to its line, so that it does not jump under
        //! the mouse on the first move.
        property double grabOffset: 0

        onPressed: mouse => {
            grabOffset = mouse.x - root.lineX;
        }
        onPositionChanged: mouse => {
            if (!pressed || !root.parent || root.parent.width <= 0) {
                return;
            }
            const position = (root.x + mouse.x - grabOffset) / root.parent.width;
            root.moved(Math.max(root.minimumPosition, Math.min(root.maximumPosition, position)));
        }
    }
}
