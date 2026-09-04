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
    spacing: 15

    Label {
        text: qsTr("Pad Amp Envelope")
        font.bold: true
        color: themeService.accentColor
    }

    Knob {
        label: qsTr("Attack")
        mapping: "exponential"
        mapMin: 0.005
        mapMax: 10.0
        suffix: "s"
        value: samplerController.selectedPadAttack * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadAttack = v / Constants.uiInternalScaling;
        }
    }

    Knob {
        label: qsTr("Decay")
        mapping: "exponential"
        mapMin: 0.005
        mapMax: 10.0
        suffix: "s"
        value: samplerController.selectedPadDecay * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadDecay = v / Constants.uiInternalScaling;
        }
    }

    Knob {
        label: qsTr("Sustain")
        value: samplerController.selectedPadSustain * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadSustain = v / Constants.uiInternalScaling;
        }
    }

    Knob {
        label: qsTr("Release")
        mapping: "exponential"
        mapMin: 0.005
        mapMax: 10.0
        suffix: "s"
        value: samplerController.selectedPadRelease * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadRelease = v / Constants.uiInternalScaling;
        }
    }

    // The offsets sit here rather than among the pad settings for the width: a second and a
    // millisecond box side by side need a column to themselves.
    SamplerDialog_Offsets {
        Layout.fillWidth: true
    }
}
