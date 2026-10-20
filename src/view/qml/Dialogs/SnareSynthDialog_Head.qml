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
        text: qsTr("Head / Membrane")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Tune")
        value: snareSynthController.tune
        onMoved: v => snareSynthController.tune = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Tone")
        value: snareSynthController.tone
        onMoved: v => snareSynthController.tone = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay")
        value: snareSynthController.decay
        onMoved: v => snareSynthController.decay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Head Tension")
        value: snareSynthController.headTension
        onMoved: v => snareSynthController.headTension = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pitch Depth")
        value: snareSynthController.pitchDepth
        onMoved: v => snareSynthController.pitchDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pitch Decay")
        value: snareSynthController.pitchDecay
        onMoved: v => snareSynthController.pitchDecay = v
        Layout.fillWidth: true
    }
}
