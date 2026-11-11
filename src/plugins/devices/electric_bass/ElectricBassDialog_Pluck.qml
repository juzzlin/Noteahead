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
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("Pluck")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Position")
        mapping: "exponential"
        mapMin: 40
        mapMax: 300
        suffix: "mm"
        value: electricBassController.position
        onMoved: v => electricBassController.position = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("Where the finger plucks, from the bridge: close to it is tight and bright, towards the neck round and hollow.")
    }
    Knob {
        label: qsTr("Finger")
        value: electricBassController.finger
        onMoved: v => electricBassController.finger = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The finger plucking: soft flesh rounds off the top, a hard fingertip brings it out.")
    }
}
