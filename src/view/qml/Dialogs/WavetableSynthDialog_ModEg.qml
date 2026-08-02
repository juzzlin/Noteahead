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
        text: qsTr("Mod Envelope")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Attack")
        mapping: "exponential"
        mapMin: 0.001
        mapMax: 10.0
        suffix: "s"
        value: wavetableSynthController.modAttack
        onMoved: v => wavetableSynthController.modAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        mapping: "exponential"
        mapMin: 0.01
        mapMax: 10.0
        suffix: "s"
        value: wavetableSynthController.modDecay
        onMoved: v => wavetableSynthController.modDecay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain")
        value: wavetableSynthController.modSustain
        onMoved: v => wavetableSynthController.modSustain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Intensity")
        mapping: "cubicCentered"
        mapMin: -100
        mapMax: 100
        value: wavetableSynthController.modInt
        onMoved: v => wavetableSynthController.modInt = v
        Layout.fillWidth: true
    }
    ComboBox {
        model: wavetableSynthController.modTargetNames
        currentIndex: wavetableSynthController.modTarget
        onActivated: i => wavetableSynthController.modTarget = i
        Layout.fillWidth: true
    }
}
