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
GroupBox {
    id: root

    property string voiceName: ""

    title: qsTr("Amp Envelope") + (voiceName === "" ? "" : " (" + voiceName + ")")
    Layout.fillWidth: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    ScrollView {
        anchors.fill: parent
        contentWidth: ampEgRow.implicitWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff

        RowLayout {
            id: ampEgRow
            spacing: 15

            Knob {
                label: qsTr("Attack")
                mapping: "cubic"
                mapMin: 0
                mapMax: 1.0
                suffix: "s"
                value: drumSynthV2Controller.voiceAmpAttack
                onMoved: val => drumSynthV2Controller.voiceAmpAttack = val
            }
            Knob {
                label: qsTr("Hold")
                mapping: "cubic"
                mapMin: 0
                mapMax: 8.0
                suffix: "s"
                value: drumSynthV2Controller.voiceAmpHold
                onMoved: val => drumSynthV2Controller.voiceAmpHold = val
            }
            Knob {
                label: qsTr("Decay")
                mapping: "exponential"
                mapMin: 0.005
                mapMax: 8.0
                suffix: "s"
                value: drumSynthV2Controller.voiceAmpDecay
                onMoved: val => drumSynthV2Controller.voiceAmpDecay = val
            }
            Knob {
                label: qsTr("Curve")
                value: drumSynthV2Controller.voiceAmpCurve
                onMoved: val => drumSynthV2Controller.voiceAmpCurve = val
            }
        }
    }
}
