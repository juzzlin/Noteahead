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
        text: qsTr("Head")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Tune")
        mapping: "exponential"
        mapMin: 25
        mapMax: 400
        suffix: "Hz"
        value: kickSynthController.tune
        onMoved: v => kickSynthController.tune = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The pitch the head settles to after the hit.")
    }
    Knob {
        label: qsTr("Decay")
        mapping: "exponential"
        mapMin: 0.1
        mapMax: 8
        suffix: "s"
        value: kickSynthController.decay
        onMoved: v => kickSynthController.decay = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the head rings with no damping.")
    }
    Knob {
        label: qsTr("Tone")
        value: kickSynthController.tone
        onMoved: v => kickSynthController.tone = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long the higher modes ring against the fundamental: low is dark and round, high is bright.")
    }
    Knob {
        label: qsTr("Pitch Drop")
        value: kickSynthController.pitchDrop
        onMoved: v => kickSynthController.pitchDrop = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How sharp a hard hit starts before the head settles. Follows the hit: a soft one starts less sharp.")
    }
    Knob {
        label: qsTr("Damping")
        value: kickSynthController.damping
        onMoved: v => kickSynthController.damping = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("A pillow against the heads: shortens the drum the way a real one does, faster than it would die on its own.")
    }
    Knob {
        label: qsTr("Ring")
        value: kickSynthController.ring
        onMoved: v => kickSynthController.ring = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The upper modes of the head: the broad ring above the thump that outlasts the beater.")
    }
    Knob {
        label: qsTr("Ring Decay")
        mapping: "exponential"
        mapMin: 0.03
        mapMax: 1.5
        suffix: "s"
        value: kickSynthController.ringDecay
        onMoved: v => kickSynthController.ringDecay = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How long that ring lasts.")
    }
}
