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
        text: qsTr("VCF")
        Layout.topMargin: 10
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Slope")
        }
        ComboBox {
            model: ["12 dB (2-pole)", "24 dB (4-pole)"]
            currentIndex: juSynth08Controller.vcfSlope
            onActivated: i => juSynth08Controller.vcfSlope = i
            Layout.fillWidth: true
        }
    }
    FilterKnob {
        label: qsTr("Cutoff")
        controller: juSynth08Controller
        value: juSynth08Controller.lpfCutoff
        onMoved: v => juSynth08Controller.lpfCutoff = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Resonance")
        value: juSynth08Controller.lpfResonance
        onMoved: v => juSynth08Controller.lpfResonance = v
        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Env Mod")
            value: juSynth08Controller.envMod
            onMoved: v => juSynth08Controller.envMod = v
            Layout.fillWidth: true
        }
        CheckBox {
            text: qsTr("Invert")
            checked: juSynth08Controller.envPolarity
            onToggled: juSynth08Controller.envPolarity = checked
        }
    }
    Knob {
        label: qsTr("LFO Mod")
        value: juSynth08Controller.filterLfoDepth
        onMoved: v => juSynth08Controller.filterLfoDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Key Track")
        value: juSynth08Controller.keyTrack
        onMoved: v => juSynth08Controller.keyTrack = v
        Layout.fillWidth: true
    }
}
