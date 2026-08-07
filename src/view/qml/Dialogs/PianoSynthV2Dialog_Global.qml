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

    Label {
        text: qsTr("Global")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: pianoSynthV2Controller.gain
        onMoved: v => pianoSynthV2Controller.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: pianoSynthV2Controller.volume
        onMoved: v => pianoSynthV2Controller.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: pianoSynthV2Controller.pan
        onMoved: v => pianoSynthV2Controller.pan = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Stereo Width")
        value: pianoSynthV2Controller.stereoWidth
        onMoved: v => pianoSynthV2Controller.stereoWidth = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: pianoSynthV2Controller
        value: pianoSynthV2Controller.lpfCutoff
        onMoved: v => pianoSynthV2Controller.lpfCutoff = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: pianoSynthV2Controller
        isHpf: true
        value: pianoSynthV2Controller.hpfCutoff
        onMoved: v => pianoSynthV2Controller.hpfCutoff = v
        Layout.fillWidth: true
    }
}
