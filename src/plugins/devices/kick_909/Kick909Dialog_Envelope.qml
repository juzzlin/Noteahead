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

//! The amp envelope's knobs; Kick909Dialog_Waveform draws it over a hit. The envelope shapes what
//! the drum plays rather than replacing it: at its defaults, Sustain at full, it does nothing at all.
ColumnLayout {
    id: root

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("Amp Envelope")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Attack")
        mapping: "cubic"
        mapMin: 0
        mapMax: 1.0
        suffix: "s"
        value: kick909Controller.ampAttack
        onMoved: v => kick909Controller.ampAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Hold")
        mapping: "cubic"
        mapMin: 0
        mapMax: 8.0
        suffix: "s"
        value: kick909Controller.ampHold
        onMoved: v => kick909Controller.ampHold = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        mapping: "exponential"
        mapMin: 0.005
        mapMax: 8.0
        suffix: "s"
        value: kick909Controller.ampDecay
        onMoved: v => kick909Controller.ampDecay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain")
        value: kick909Controller.ampSustain
        onMoved: v => kick909Controller.ampSustain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release")
        mapping: "exponential"
        mapMin: 0.005
        mapMax: 8.0
        suffix: "s"
        value: kick909Controller.ampRelease
        onMoved: v => kick909Controller.ampRelease = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Curve")
        value: kick909Controller.ampCurve
        onMoved: v => kick909Controller.ampCurve = v
        Layout.fillWidth: true
    }
}
