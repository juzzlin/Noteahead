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
        text: qsTr("Amp EG")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Attack")
        mapping: "exponential"
        mapMin: 0.001
        mapMax: 10.0
        suffix: "s"
        value: fmSynthController.ampAttack
        onMoved: v => fmSynthController.ampAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        mapping: "exponential"
        mapMin: 0.01
        mapMax: 10.0
        suffix: "s"
        value: fmSynthController.ampDecay
        onMoved: v => fmSynthController.ampDecay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain")
        value: fmSynthController.ampSustain
        onMoved: v => fmSynthController.ampSustain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release")
        mapping: "exponential"
        mapMin: 0.01
        mapMax: 10.0
        suffix: "s"
        value: fmSynthController.ampRelease
        onMoved: v => fmSynthController.ampRelease = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Curve")
        value: fmSynthController.ampCurve
        onMoved: v => fmSynthController.ampCurve = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Velocity Sensitivity")
        value: fmSynthController.ampVelocitySensitivity
        onMoved: v => fmSynthController.ampVelocitySensitivity = v
        Layout.fillWidth: true
    }
}
