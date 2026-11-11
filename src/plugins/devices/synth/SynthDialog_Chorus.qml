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

// The rack Chorus, built in ahead of the delay. The readouts follow what the effect does with each
// control: Rate 0.1 to 10 Hz in equal ratios, Delay 5 to 30 ms.
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("Chorus Effect")
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    // Two rows rather than three, so that Mix is in view without scrolling the tab.
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Rate")
            suffix: "Hz"
            mapping: "exponential"
            mapMin: 0.1
            mapMax: 10.0
            value: synthController.chorusRate
            onMoved: v => synthController.chorusRate = v
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Knob {
            label: qsTr("Depth")
            value: synthController.chorusDepth
            onMoved: v => synthController.chorusDepth = v
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Knob {
            label: qsTr("Delay")
            suffix: "ms"
            mapping: "linear"
            mapMin: 5.0
            mapMax: 30.0
            value: synthController.chorusDelay
            onMoved: v => synthController.chorusDelay = v
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Width")
            value: synthController.chorusWidth
            onMoved: v => synthController.chorusWidth = v
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
        Knob {
            label: qsTr("Mix")
            value: synthController.chorusMix
            onMoved: v => synthController.chorusMix = v
            Layout.fillWidth: true
            Layout.preferredWidth: 1
        }
    }
}
