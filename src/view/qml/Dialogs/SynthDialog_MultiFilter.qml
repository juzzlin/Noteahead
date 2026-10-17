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

// The multi engine's own filter section, the same as each oscillator's. A module of its own rather
// than a page behind a tab: the noise tab has the room, so nothing needs hiding.
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("Multi Filter")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        spacing: 10
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("LPF Slope")
            }
            ComboBox {
                model: ["12 dB/oct", "24 dB/oct"]
                currentIndex: synthController.multiLpfSlope
                onActivated: idx => synthController.multiLpfSlope = idx
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("HPF Slope")
            }
            ComboBox {
                model: ["12 dB/oct", "24 dB/oct"]
                currentIndex: synthController.multiHpfSlope
                onActivated: idx => synthController.multiHpfSlope = idx
                Layout.fillWidth: true
            }
        }
    }

    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: synthController
        value: synthController.multiLpfCutoff
        onMoved: v => synthController.multiLpfCutoff = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Rolls the top off the multi engine alone, before it reaches the mix and the voice's own filter.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("LPF Resonance")
        value: synthController.multiLpfResonance
        onMoved: v => synthController.multiLpfResonance = v
        Layout.fillWidth: true
    }
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: synthController
        value: synthController.multiHpfCutoff
        isHpf: true
        onMoved: v => synthController.multiHpfCutoff = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Thins out the multi engine alone, which is how noise is made to sit over the oscillators rather than across them.")
        Layout.fillWidth: true
    }
}
