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
        value: pianoSynthAcousticController.gain
        onMoved: v => pianoSynthAcousticController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: pianoSynthAcousticController.volume
        onMoved: v => pianoSynthAcousticController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: pianoSynthAcousticController.pan
        onMoved: v => pianoSynthAcousticController.pan = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan Spread")
        value: pianoSynthAcousticController.panSpread
        onMoved: v => pianoSynthAcousticController.panSpread = v
        Layout.fillWidth: true
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("How far the keyboard is spread across the stereo field: the bass to the left, the treble to the right, as the player hears it.")
    }
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: pianoSynthAcousticController
        value: pianoSynthAcousticController.lpfCutoff
        onMoved: v => pianoSynthAcousticController.lpfCutoff = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: pianoSynthAcousticController
        isHpf: true
        value: pianoSynthAcousticController.hpfCutoff
        onMoved: v => pianoSynthAcousticController.hpfCutoff = v
        Layout.fillWidth: true
    }
    OctaveKnob {
        controller: pianoSynthAcousticController
    }
}
