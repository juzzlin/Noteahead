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
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"

//! The selected pad's noise: what FM cannot rattle on its own. Heard directly, and it can also
//! modulate one operator, which roughens that operator while keeping it on the drum's pitch.
ColumnLayout {
    id: root

    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 6

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    Label {
        text: qsTr("Noise")
        font.bold: true
        color: themeService.accentColor
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Filter")
            }
            ComboBox {
                Layout.fillWidth: true
                model: fmDrumsController.noiseFilterModeNames
                currentIndex: fmDrumsController.noiseFilterMode
                onActivated: i => fmDrumsController.noiseFilterMode = i
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("FM Target")
            }
            ComboBox {
                Layout.fillWidth: true
                // Operators as the panels title them
                model: ["Op 1", "Op 2", "Op 3", "Op 4"]
                currentIndex: fmDrumsController.noiseTarget
                onActivated: i => fmDrumsController.noiseTarget = i
            }
        }
    }

    GridLayout {
        columns: 2
        columnSpacing: 10
        rowSpacing: 6
        Layout.fillWidth: true

        Knob {
            label: qsTr("Level")
            value: fmDrumsController.noiseLevel
            onMoved: v => fmDrumsController.noiseLevel = v
        }
        Knob {
            label: qsTr("FM")
            value: fmDrumsController.noiseModulation
            onMoved: v => fmDrumsController.noiseModulation = v
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How much the noise modulates the target operator's phase.")
        }
        Knob {
            label: qsTr("Cutoff")
            mapping: "exponential"
            mapMin: 20
            mapMax: 20000
            suffix: "Hz"
            value: fmDrumsController.noiseCutoff
            onMoved: v => fmDrumsController.noiseCutoff = v
        }
        Knob {
            label: qsTr("Reso")
            value: fmDrumsController.noiseResonance
            onMoved: v => fmDrumsController.noiseResonance = v
        }
        Knob {
            label: qsTr("Attack")
            mapping: "cubic"
            mapMin: 0
            mapMax: 1.0
            suffix: "s"
            value: fmDrumsController.noiseAttack
            onMoved: v => fmDrumsController.noiseAttack = v
        }
        Knob {
            label: qsTr("Decay")
            mapping: "exponential"
            mapMin: 0.005
            mapMax: 8.0
            suffix: "s"
            value: fmDrumsController.noiseDecay
            onMoved: v => fmDrumsController.noiseDecay = v
        }
        Knob {
            label: qsTr("Curve")
            value: fmDrumsController.noiseCurve
            onMoved: v => fmDrumsController.noiseCurve = v
        }
    }
}
