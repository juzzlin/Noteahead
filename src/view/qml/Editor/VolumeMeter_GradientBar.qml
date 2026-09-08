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

// The classic volume meter: one continuous bar under a red/yellow/green gradient, clipped to the
// level. See VolumeMeter.qml for the animation that drives it.
Item {
    id: rootItem
    property real level: 0.0 // Normalized level (0.0 to 1.0)
    property real maxHeight: 1.0
    Item {
        id: levelIndicatorGradientClip
        width: parent.width * 0.2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        height: parent.height * rootItem.level * rootItem.maxHeight
        clip: true
        Rectangle {
            id: levelIndicatorGradient
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            height: rootItem.height * rootItem.maxHeight
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: "red"
                }
                GradientStop {
                    position: 0.5
                    color: "yellow"
                }
                GradientStop {
                    position: 1.0
                    color: "green"
                }
            }
        }
        Rectangle {
            id: leftShadow
            width: levelIndicatorGradient.width * 0.2
            height: levelIndicatorGradient.height
            anchors.left: levelIndicatorGradient.left
            opacity: 0.25
            color: "white"
        }
        Rectangle {
            id: rightShadow
            width: leftShadow.width
            height: leftShadow.height
            anchors.right: levelIndicatorGradient.right
            opacity: leftShadow.opacity
            color: "black"
        }
    }
}
