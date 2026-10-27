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
        Layout.topMargin: 10
    }
    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: noiseFxController.gain
        onMoved: v => noiseFxController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: noiseFxController.volume
        onMoved: v => noiseFxController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: noiseFxController.pan
        onMoved: v => noiseFxController.pan = v
        Layout.fillWidth: true
    }
    OctaveKnob {
        controller: noiseFxController
    }
    CheckBox {
        text: qsTr("Chromatic")
        checked: noiseFxController.chromatic
        onToggled: noiseFxController.chromatic = checked
        ToolTip.visible: hovered
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.text: qsTr("Plays the patch like a sampler: an octave up is an octave higher and twice as fast")
    }
}
