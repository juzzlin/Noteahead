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
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: snareSynthController.gain
        onMoved: v => snareSynthController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
        value: snareSynthController.volume
        onMoved: v => snareSynthController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: snareSynthController.pan
        onMoved: v => snareSynthController.pan = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan Spread")
        value: snareSynthController.panSpread
        onMoved: v => snareSynthController.panSpread = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Drive")
        value: snareSynthController.drive
        onMoved: v => snareSynthController.drive = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: snareSynthController
        value: snareSynthController.lpfCutoff
        onMoved: v => snareSynthController.lpfCutoff = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: snareSynthController
        isHpf: true
        value: snareSynthController.hpfCutoff
        onMoved: v => snareSynthController.hpfCutoff = v
        Layout.fillWidth: true
    }
    OctaveKnob {
        controller: snareSynthController
    }
    CheckBox {
        text: qsTr("Key Track")
        checked: snareSynthController.keyTrack
        onToggled: snareSynthController.keyTrack = checked
        Layout.fillWidth: true
        Layout.topMargin: 5
    }
}
