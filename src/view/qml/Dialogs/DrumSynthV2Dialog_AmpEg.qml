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

//! The per-voice amp envelope: what the voice's own engine plays is shaped by this rather than
//! replaced by it, so pulling Hold down is how a drum is tightened.
ColumnLayout {
    id: root

    property string voiceName: ""

    Layout.fillWidth: true
    spacing: 10

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    // A heading rather than a framed box, as the Sampler and every other device dialog head their
    // sections: the accent colour is what says "section", not a rectangle around it.
    Label {
        text: qsTr("Amp Envelope") + (root.voiceName === "" ? "" : " (" + root.voiceName + ")")
        font.bold: true
        color: themeService.accentColor
    }


        Knob {
            Layout.fillWidth: true
            label: qsTr("Attack")
            mapping: "cubic"
            mapMin: 0
            mapMax: 1.0
            suffix: "s"
            value: drumSynthV2Controller.voiceAmpAttack
            onMoved: val => drumSynthV2Controller.voiceAmpAttack = val
        }
        Knob {
            Layout.fillWidth: true
            label: qsTr("Hold")
            mapping: "cubic"
            mapMin: 0
            mapMax: 8.0
            suffix: "s"
            value: drumSynthV2Controller.voiceAmpHold
            onMoved: val => drumSynthV2Controller.voiceAmpHold = val
        }
        Knob {
            Layout.fillWidth: true
            label: qsTr("Decay")
            mapping: "exponential"
            mapMin: 0.005
            mapMax: 8.0
            suffix: "s"
            value: drumSynthV2Controller.voiceAmpDecay
            onMoved: val => drumSynthV2Controller.voiceAmpDecay = val
        }
        Knob {
            Layout.fillWidth: true
            label: qsTr("Sustain")
            value: drumSynthV2Controller.voiceAmpSustain
            onMoved: val => drumSynthV2Controller.voiceAmpSustain = val
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Where the decay lands. At zero the voice decays to silence as a one-shot drum does; at full the envelope stops shaping the voice at all, which is what makes V2 sound exactly like V1.")
        }
        Knob {
            Layout.fillWidth: true
            label: qsTr("Release")
            mapping: "exponential"
            mapMin: 0.005
            mapMax: 8.0
            suffix: "s"
            value: drumSynthV2Controller.voiceAmpRelease
            onMoved: val => drumSynthV2Controller.voiceAmpRelease = val
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How the voice falls away once the note ends. Nothing to hear unless Sustain is above zero, since a voice with none has already gone quiet by then.")
        }
        Knob {
            Layout.fillWidth: true
            label: qsTr("Curve")
            value: drumSynthV2Controller.voiceAmpCurve
            onMoved: val => drumSynthV2Controller.voiceAmpCurve = val
        }
}
