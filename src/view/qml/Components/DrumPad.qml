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
import QtQuick.Layouts 1.15
import Noteahead 1.0

// One voice of a drum machine, drawn as the Sampler draws a pad: a rounded rectangle that fills
// with the accent colour when it is the selected one. Shared by both Drum Synths so the two cannot
// drift apart, which is how they came to look different from the Sampler in the first place.
Rectangle {
    id: root

    required property string label
    required property bool selected
    //! Raised when the pad is struck, which selects it as well as playing it.
    signal struck
    //! Raised by the small FX button in the corner.
    signal effectsRequested

    // Nearly square, so that three rows of them stand about as tall as the column of global
    // controls beside them. Still wider than tall, because a drum's name needs the width.
    implicitWidth: 112
    implicitHeight: 96
    radius: 12

    // The accent colour is the background on the selected pad, so the text has to be the one that
    // contrasts with it rather than a fixed white.
    readonly property color textColor: root.selected ? themeService.accentTextColor : "#dddddd"
    color: root.selected ? themeService.accentColor : "#333333"
    border.color: root.selected ? themeService.accentColor : (mouseArea.pressed ? "white" : "#555")
    border.width: root.selected ? 3 : 2

    Label {
        anchors.centerIn: parent
        // The pad is only as wide as it is, and a drum's name can be longer than a note's.
        width: parent.width - 16
        text: root.label
        color: root.textColor
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.struck()
    }

    AppButton {
        text: qsTr("FX")
        // As on a Sampler pad: the selected pad's background is the accent colour, and the style's
        // white label all but disappears on a bright one.
        Universal.foreground: root.textColor
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        implicitWidth: 28
        implicitHeight: 18
        padding: 0
        font.pointSize: 8
        z: 10
        onClicked: root.effectsRequested()
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Insert effects and sends for this voice")
    }
}
