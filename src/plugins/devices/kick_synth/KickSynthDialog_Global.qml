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
        value: kickSynthController.gain
        onMoved: v => kickSynthController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: kickSynthController.volume
        onMoved: v => kickSynthController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: kickSynthController.pan
        onMoved: v => kickSynthController.pan = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Drive")
        value: kickSynthController.drive
        onMoved: v => kickSynthController.drive = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Velocity Sensitivity")
        value: kickSynthController.velocitySensitivity
        onMoved: v => kickSynthController.velocitySensitivity = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How much softer notes play softer. A soft hit also starts less sharp and clicks less, as a real kick does.")
    }
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: kickSynthController
        value: kickSynthController.lpfCutoff
        onMoved: v => kickSynthController.lpfCutoff = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: kickSynthController
        isHpf: true
        value: kickSynthController.hpfCutoff
        onMoved: v => kickSynthController.hpfCutoff = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Chromatic")
        checked: kickSynthController.chromatic
        onToggled: kickSynthController.chromatic = checked
        Layout.fillWidth: true
        Layout.topMargin: 5
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("Plays the drum like a sampler: an octave up is an octave higher and twice as short")
    }
}
