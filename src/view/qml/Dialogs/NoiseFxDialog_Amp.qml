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
        text: qsTr("Amp")
        Layout.topMargin: 10
    }
    Knob {
        label: qsTr("Attack")
        suffix: "ms"
        mapping: "exponential"
        mapMin: 1
        mapMax: 10000
        value: noiseFxController.ampAttack
        onMoved: v => noiseFxController.ampAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        suffix: "ms"
        mapping: "exponential"
        mapMin: 5
        mapMax: 20000
        value: noiseFxController.ampDecay
        onMoved: v => noiseFxController.ampDecay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain")
        value: noiseFxController.ampSustain
        onMoved: v => noiseFxController.ampSustain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release")
        suffix: "ms"
        mapping: "exponential"
        mapMin: 5
        mapMax: 20000
        value: noiseFxController.ampRelease
        onMoved: v => noiseFxController.ampRelease = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Velocity")
        value: noiseFxController.velocitySensitivity
        onMoved: v => noiseFxController.velocitySensitivity = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Play to End")
        checked: noiseFxController.playToEnd
        onToggled: noiseFxController.playToEnd = checked
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("A note plays until Sweep A has run its course, whenever its note-off comes")
    }
}
