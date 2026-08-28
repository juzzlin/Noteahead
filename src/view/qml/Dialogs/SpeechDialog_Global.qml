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
        value: speechController.gain
        onMoved: v => speechController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: speechController.volume
        onMoved: v => speechController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: speechController.pan
        onMoved: v => speechController.pan = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Velocity Sensitivity")
        value: speechController.velocitySensitivity
        onMoved: v => speechController.velocitySensitivity = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How much the note's velocity scales the level. At zero every note speaks at full level.")
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: speechController
        value: speechController.lpfCutoff
        onMoved: v => speechController.lpfCutoff = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: speechController
        isHpf: true
        value: speechController.hpfCutoff
        onMoved: v => speechController.hpfCutoff = v
        Layout.fillWidth: true
    }

    Item {
        Layout.fillHeight: true
    }
}
