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
import ".."

Item {
    id: rootItem
    readonly property int minValue: 0
    readonly property int maxValue: 100
    property int value: 100
    property string toolTipText
    //! 100 % means "no scaling at all", which is the default on every track and column. Drawing
    //! that state flat and dim keeps a fresh song quiet and lets an actually scaled track stand
    //! out, rather than filling every header with a fully saturated bar.
    readonly property bool _neutral: value >= maxValue
    signal clicked
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hoverHandler.hovered
    ToolTip.text: qsTr("%1 %").arg(value) + "\n" + toolTipText
    Rectangle {
        id: trough
        color: themeService.velocityScaleTroughColor
        radius: 2
        width: Math.max(6, rootItem.width * 0.4)
        height: rootItem.height * 0.8
        anchors.centerIn: parent
        clip: true
        Rectangle {
            id: bar
            color: themeService.accentColor
            // Hovering lifts the neutral bar part way, so the control still answers the pointer
            opacity: rootItem._neutral ? (hoverHandler.hovered ? 0.6 : 0.25) : 1
            radius: parent.radius
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * (rootItem.value - rootItem.minValue) / (rootItem.maxValue - rootItem.minValue)
            Behavior on height {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: 120
                }
            }
        }
    }
    HoverHandler {
        id: hoverHandler
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: rootItem.clicked()
        cursorShape: Qt.PointingHandCursor
    }
}
