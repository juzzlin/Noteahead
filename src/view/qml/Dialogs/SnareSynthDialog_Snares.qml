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
        text: qsTr("Snare Wires")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Snappy")
        value: snareSynthController.snappy
        onMoved: v => snareSynthController.snappy = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Snare Decay")
        value: snareSynthController.snareDecay
        onMoved: v => snareSynthController.snareDecay = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Snare Tension")
        value: snareSynthController.snareTension
        onMoved: v => snareSynthController.snareTension = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Snare Buzz")
        value: snareSynthController.snareBuzz
        onMoved: v => snareSynthController.snareBuzz = v
        Layout.fillWidth: true
    }
}
