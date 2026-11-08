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
        text: qsTr("String")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Sustain")
        mapping: "exponential"
        mapMin: 2
        mapMax: 40
        suffix: "s"
        value: electricBassController.sustain
        onMoved: v => electricBassController.sustain = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the lowest partials ring while the note is held.")
    }
    Knob {
        label: qsTr("Brightness")
        value: electricBassController.brightness
        onMoved: v => electricBassController.brightness = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the higher partials ring against the low ones: new roundwounds are bright, old strings dull.")
    }
    Knob {
        label: qsTr("Release")
        mapping: "exponential"
        mapMin: 0.02
        mapMax: 1
        suffix: "s"
        value: electricBassController.release
        onMoved: v => electricBassController.release = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How fast the string dies when the fretting finger lifts.")
    }
}
