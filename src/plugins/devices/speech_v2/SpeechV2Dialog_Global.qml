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
        value: speechV2Controller.gain
        onMoved: v => speechV2Controller.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: speechV2Controller.volume
        onMoved: v => speechV2Controller.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: speechV2Controller.pan
        onMoved: v => speechV2Controller.pan = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan Spread")
        value: speechV2Controller.panSpread
        onMoved: v => speechV2Controller.panSpread = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Doubles the voice into stereo with subtle pitch, formant and breath spread.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Velocity Sensitivity")
        value: speechV2Controller.velocitySensitivity
        onMoved: v => speechV2Controller.velocitySensitivity = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How much the note's velocity scales the level. At zero every note speaks at full level.")
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: speechV2Controller
        value: speechV2Controller.lpfCutoff
        onMoved: v => speechV2Controller.lpfCutoff = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: speechV2Controller
        isHpf: true
        value: speechV2Controller.hpfCutoff
        onMoved: v => speechV2Controller.hpfCutoff = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pitch Bend Range")
        from: 0
        to: 24
        stepSize: 1
        suffix: ""
        value: speechV2Controller.pitchBendRange
        onMoved: v => speechV2Controller.pitchBendRange = Math.round(v)
        Layout.fillWidth: true
    }

    Item {
        Layout.fillHeight: true
    }
    OctaveKnob {
        controller: speechV2Controller
    }
}
