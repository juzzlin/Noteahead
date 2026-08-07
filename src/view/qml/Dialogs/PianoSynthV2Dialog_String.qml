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

    Label {
        text: qsTr("String")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Decay")
        value: pianoSynthV2Controller.decay
        onMoved: v => pianoSynthV2Controller.decay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Brightness")
        value: pianoSynthV2Controller.brightness
        onMoved: v => pianoSynthV2Controller.brightness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Inharmonicity")
        value: pianoSynthV2Controller.inharmonicity
        onMoved: v => pianoSynthV2Controller.inharmonicity = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Unison Detune")
        value: pianoSynthV2Controller.stringDetune
        onMoved: v => pianoSynthV2Controller.stringDetune = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Double Decay")
        value: pianoSynthV2Controller.doubleDecay
        onMoved: v => pianoSynthV2Controller.doubleDecay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Stretch Tuning")
        value: pianoSynthV2Controller.stretch
        onMoved: v => pianoSynthV2Controller.stretch = v
        Layout.fillWidth: true
    }
}
