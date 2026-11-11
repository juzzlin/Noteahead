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
    FilterKnob {
        label: qsTr("Cutoff")
        controller: juSynthController
        value: juSynthController.lpfCutoff
        onMoved: v => juSynthController.lpfCutoff = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Resonance")
        value: juSynthController.lpfResonance
        onMoved: v => juSynthController.lpfResonance = v
        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Env Mod")
            value: juSynthController.envMod
            onMoved: v => juSynthController.envMod = v
            Layout.fillWidth: true
        }
        CheckBox {
            // The envelope closing the filter instead of opening it
            text: qsTr("Invert")
            checked: juSynthController.envInverted
            onToggled: juSynthController.envInverted = checked
        }
    }
    Knob {
        label: qsTr("LFO Mod")
        value: juSynthController.filterLfoDepth
        onMoved: v => juSynthController.filterLfoDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Key Track")
        value: juSynthController.keyTrack
        onMoved: v => juSynthController.keyTrack = v
        Layout.fillWidth: true
    }
}
