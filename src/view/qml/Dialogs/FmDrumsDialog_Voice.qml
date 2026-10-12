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

//! The selected pad's FM voice as a whole: how its operators are wired, what pitch it sits at and
//! how far it falls to get there, and how hard a hit has to be to brighten it.
ColumnLayout {
    id: root

    property string voiceName: ""
    //! How many knobs to a row, so the section fits whichever column it is given.
    property int knobColumns: 2

    Layout.fillWidth: true
    spacing: 10

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    Label {
        text: qsTr("Voice") + (root.voiceName === "" ? "" : " (" + root.voiceName + ")")
        font.bold: true
        color: themeService.accentColor
    }

    // Any pad can be any drum: this loads a sound onto the selected pad and leaves its mix alone
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Sound")
        }
        ComboBox {
            Layout.fillWidth: true
            model: fmDrumsController.padPresetNames
            // A menu of sounds to load rather than a readout of the one loaded: after a knob has
            // moved, no entry would be true any more
            currentIndex: -1
            displayText: qsTr("Load a sound...")
            onActivated: index => {
                fmDrumsController.loadPadPreset(index);
                currentIndex = -1;
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Algorithm")
        }
        ComboBox {
            Layout.fillWidth: true
            model: fmDrumsController.algorithmNames
            currentIndex: fmDrumsController.algorithm
            onActivated: i => fmDrumsController.algorithm = i
        }
    }

    GridLayout {
        columns: root.knobColumns
        columnSpacing: 15
        rowSpacing: 6
        Layout.fillWidth: true

        Knob {
            label: qsTr("Pitch")
            mapping: "exponential"
            mapMin: 20
            mapMax: 5000
            suffix: "Hz"
            value: fmDrumsController.pitch
            onMoved: val => fmDrumsController.pitch = val
        }
        Knob {
            label: qsTr("Feedback")
            value: fmDrumsController.feedback
            onMoved: val => fmDrumsController.feedback = val
        }
        // The bend every operator falls through together, which is what makes a drum of a bell
        Knob {
            label: qsTr("P.Depth")
            mapping: "cubicCentered"
            mapMin: -48
            mapMax: 48
            suffix: " st"
            value: fmDrumsController.pitchDepth
            onMoved: val => fmDrumsController.pitchDepth = val
        }
        Knob {
            label: qsTr("P.Decay")
            mapping: "exponential"
            mapMin: 0.001
            mapMax: 1.0
            suffix: "s"
            value: fmDrumsController.pitchDecay
            onMoved: val => fmDrumsController.pitchDecay = val
        }
        Knob {
            label: qsTr("Velocity")
            value: fmDrumsController.velocitySensitivity
            onMoved: val => fmDrumsController.velocitySensitivity = val
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How much quieter a soft hit is than a hard one.")
        }
        Knob {
            label: qsTr("Vel > FM")
            value: fmDrumsController.modVelocitySensitivity
            onMoved: val => fmDrumsController.modVelocitySensitivity = val
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How much duller a soft hit is than a hard one: velocity scales how deep the operators modulate.")
        }
    }
}
