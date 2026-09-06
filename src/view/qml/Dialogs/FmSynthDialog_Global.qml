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
        text: qsTr("Voice / Global")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    ComboBox {
        model: fmSynthController.voiceModes
        currentIndex: fmSynthController.voiceMode
        onActivated: i => fmSynthController.voiceMode = i
        Layout.fillWidth: true
        Layout.bottomMargin: 10
    }
    Knob {
        label: qsTr("Voice Depth")
        value: fmSynthController.voiceDepth
        onMoved: v => fmSynthController.voiceDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Portamento")
        value: fmSynthController.portamento
        onMoved: v => fmSynthController.portamento = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan Spread")
        value: fmSynthController.panSpread
        onMoved: v => fmSynthController.panSpread = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pitch Bend Range")
        from: 0
        to: 24
        stepSize: 1
        suffix: ""
        value: fmSynthController.pitchBendRange
        onMoved: v => fmSynthController.pitchBendRange = Math.round(v)
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: fmSynthController.gain
        onMoved: v => fmSynthController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: fmSynthController.volume
        onMoved: v => fmSynthController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: fmSynthController.pan
        onMoved: v => fmSynthController.pan = v
        Layout.fillWidth: true
    }
}
