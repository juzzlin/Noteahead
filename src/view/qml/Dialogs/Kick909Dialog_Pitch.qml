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

// The two knobs the RD-9 adds to the 909's bass drum
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("Pitch")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Pitch")
        mapping: "exponential"
        mapMin: 30
        mapMax: 120
        suffix: "Hz"
        value: kick909Controller.pitch
        onMoved: v => kick909Controller.pitch = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The pitch the drum settles to. The sweep moves with it. The 909's own is 49.5 Hz.")
    }
    Knob {
        label: qsTr("Pitch Depth")
        mapping: "value"
        mapMin: 0
        mapMax: 200
        suffix: "%"
        value: kick909Controller.pitchDepth
        onMoved: v => kick909Controller.pitchDepth = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How far above its pitch the drum starts: 100 % is the 909's own sweep, 0 % none at all.")
    }
}
