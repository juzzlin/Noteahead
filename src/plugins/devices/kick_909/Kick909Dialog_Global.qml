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
        text: qsTr("Global")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: kick909Controller.gain
        onMoved: v => kick909Controller.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: kick909Controller.volume
        onMoved: v => kick909Controller.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: kick909Controller.pan
        onMoved: v => kick909Controller.pan = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Drive")
        value: kick909Controller.drive
        onMoved: v => kick909Controller.drive = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("Overdrives the drum towards a square wave, for the hard techno and gabber kick. The level is held about the same.")
    }
    Knob {
        label: qsTr("Velocity Sensitivity")
        value: kick909Controller.velocitySensitivity
        onMoved: v => kick909Controller.velocitySensitivity = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How much softer notes play softer")
    }
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: kick909Controller
        value: kick909Controller.lpfCutoff
        onMoved: v => kick909Controller.lpfCutoff = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: kick909Controller
        isHpf: true
        value: kick909Controller.hpfCutoff
        onMoved: v => kick909Controller.hpfCutoff = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Chromatic")
        checked: kick909Controller.chromatic
        onToggled: kick909Controller.chromatic = checked
        Layout.fillWidth: true
        Layout.topMargin: 5
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("The note moves Pitch, C-3 playing the drum as it is set. The decays stay as they are.")
    }
}
