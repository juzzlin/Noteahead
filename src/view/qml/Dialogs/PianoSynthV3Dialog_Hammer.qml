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
        text: qsTr("Hammer & Dampers")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Hammer Hardness")
        value: pianoSynthV3Controller.hammerHardness
        onMoved: v => pianoSynthV3Controller.hammerHardness = v
        Layout.fillWidth: true
    }
    Knob {
        // The ramp the strike opens with. Centre is the one the model was fitted to; below it the
        // note arrives more abruptly, above it the hammer is felt rather than heard.
        label: qsTr("Attack")
        value: pianoSynthV3Controller.attack
        onMoved: v => pianoSynthV3Controller.attack = v
        Layout.fillWidth: true
    }
    Knob {
        // Full is the reference's own law, where the level follows velocity squared. Turning it
        // down lifts the softer strikes toward the loud ones without touching their tone, which
        // is what makes the top of the keyboard playable without hammering it.
        label: qsTr("Velocity Sensitivity")
        value: pianoSynthV3Controller.velocitySensitivity
        onMoved: v => pianoSynthV3Controller.velocitySensitivity = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Richness")
        value: pianoSynthV3Controller.richness
        onMoved: v => pianoSynthV3Controller.richness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release Time")
        value: pianoSynthV3Controller.releaseTime
        onMoved: v => pianoSynthV3Controller.releaseTime = v
        Layout.fillWidth: true
    }
}
