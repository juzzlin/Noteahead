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
        text: qsTr("Noise")
        Layout.topMargin: 10
    }
    Knob {
        // The noise alone: the crackle keeps its level
        label: qsTr("Level")
        value: noiseFxController.noiseLevel
        onMoved: v => noiseFxController.noiseLevel = v
        Layout.fillWidth: true
    }
    Knob {
        // Brown at the bottom, pink, white, and blue at the top
        label: qsTr("Colour")
        value: noiseFxController.colour
        onMoved: v => noiseFxController.colour = v
        Layout.fillWidth: true
    }
    Knob {
        // At the top every sample passes
        label: qsTr("S&H Rate")
        suffix: "Hz"
        mapping: "exponential"
        mapMin: 50
        mapMax: 50000
        value: noiseFxController.sampleHoldRate
        onMoved: v => noiseFxController.sampleHoldRate = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Crackle")
        value: noiseFxController.crackle
        onMoved: v => noiseFxController.crackle = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Width")
        value: noiseFxController.width
        onMoved: v => noiseFxController.width = v
        Layout.fillWidth: true
    }
}
