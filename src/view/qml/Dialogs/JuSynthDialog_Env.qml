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

// The one envelope the filter and the amplifier share
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("ENV")
        Layout.topMargin: 10
    }
    Knob {
        label: qsTr("Attack")
        suffix: "ms"
        mapping: "exponential"
        mapMin: 1
        mapMax: 3000
        value: juSynthController.attack
        onMoved: v => juSynthController.attack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        suffix: "ms"
        mapping: "exponential"
        mapMin: 2
        mapMax: 12000
        value: juSynthController.decay
        onMoved: v => juSynthController.decay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain")
        value: juSynthController.sustain
        onMoved: v => juSynthController.sustain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release")
        suffix: "ms"
        mapping: "exponential"
        mapMin: 2
        mapMax: 12000
        value: juSynthController.release
        onMoved: v => juSynthController.release = v
        Layout.fillWidth: true
    }
}
