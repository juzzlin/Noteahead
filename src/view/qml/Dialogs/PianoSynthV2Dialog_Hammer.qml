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
        text: qsTr("Hammer & Dampers")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Hammer Hardness")
        value: pianoSynthV2Controller.hammerHardness
        onMoved: v => pianoSynthV2Controller.hammerHardness = v
        Layout.fillWidth: true
    }
    Knob {
        // The ramp the strike opens with. Centre is the one the model was fitted to; below it the
        // note arrives more abruptly, above it the hammer is felt rather than heard.
        label: qsTr("Attack")
        value: pianoSynthV2Controller.attack
        onMoved: v => pianoSynthV2Controller.attack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Richness")
        value: pianoSynthV2Controller.richness
        onMoved: v => pianoSynthV2Controller.richness = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release Time")
        value: pianoSynthV2Controller.releaseTime
        onMoved: v => pianoSynthV2Controller.releaseTime = v
        Layout.fillWidth: true
    }
}
