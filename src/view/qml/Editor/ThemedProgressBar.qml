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

Item {
    id: rootItem
    opacity: 0
    visible: false
    property alias value: control.value
    z: 100
    function reset(): void {
        fadeOutAnimation.stop();
        rootItem.opacity = 1;
        rootItem.visible = true;
        control.value = 0;
    }
    function fadeOut(): void {
        fadeOutAnimation.start();
    }
    Rectangle {
        id: shadowRect
        anchors.fill: parent
        anchors.margins: -20
        color: themeService.progressBarBackgroundColor
        radius: 10
        opacity: 0.9
    }
    ProgressBar {
        id: control
        anchors.fill: parent
        value: 0
        from: 0
        to: 1
        background: Rectangle {
            implicitWidth: 200
            implicitHeight: 6
            color: themeService.progressBarBackgroundColor
            radius: 3
        }
        contentItem: Item {
            implicitWidth: 200
            implicitHeight: 6
            Rectangle {
                width: control.visualPosition * parent.width
                height: parent.height
                radius: 3
                color: themeService.accentColor
            }
        }
    }
    NumberAnimation {
        id: fadeOutAnimation
        target: rootItem
        property: "opacity"
        to: 0
        duration: 500
        easing.type: Easing.InCubic
        onStopped: rootItem.visible = false
    }
}
