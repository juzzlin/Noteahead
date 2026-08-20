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
        text: qsTr("VCO 3")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: synthController.vcoWaveformNames
            currentIndex: synthController.vco3Waveform
            onActivated: i => synthController.vco3Waveform = i
            Layout.fillWidth: true
        }
        ComboBox {
            model: synthController.octaveNames
            currentIndex: synthController.vco3Octave + 2
            onActivated: i => synthController.vco3Octave = i - 2
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Pitch")
        mapping: "cubicCentered"
        mapMin: -2400
        mapMax: 2400
        suffix: "c"
        value: synthController.vco3Pitch
        onMoved: v => synthController.vco3Pitch = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Shape")
        value: synthController.vco3Shape
        onMoved: v => synthController.vco3Shape = v
        Layout.fillWidth: true
    }
    Knob {
        // Only the pulse has edges to round; a saw's ramp is the waveform itself.
        label: qsTr("Roundness")
        value: synthController.vco3Roundness
        onMoved: v => synthController.vco3Roundness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Level")
        value: synthController.mixVco3
        onMoved: v => synthController.mixVco3 = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Hard Sync to VCO2")
        checked: synthController.vco3Sync
        onToggled: synthController.vco3Sync = checked
    }
}
