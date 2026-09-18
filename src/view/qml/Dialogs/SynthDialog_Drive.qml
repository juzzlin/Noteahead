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

// How hard the low pass is driven into its own ceiling.
//
// Not the same thing as a distortion in front of the filter, which is what an insert effect can
// already do: there the harmonics are made outside the loop that is meant to tame them, and the
// resonant peak keeps its height however hard the input is pushed. Here the fold is inside the
// loop, so the peak compresses as the filter is driven and everything the loop makes is filtered by
// the same poles on its way round again.
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    Label {
        text: qsTr("Filter Drive")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        Layout.fillWidth: true

        Knob {
            label: qsTr("Drive")
            value: synthController.filterDrive
            onMoved: v => synthController.filterDrive = v
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How hard the low pass is driven. The peaks and the resonance fold inside the filter rather than in front of it, so the tone thickens and the resonant peak compresses instead of tearing. At zero the filter is the clean one every patch was made on.")
            Layout.fillWidth: true
        }
    }

    Label {
        text: qsTr("Drive works on the main low pass, not on the per-oscillator filters. The level is held roughly where it was, so the knob changes the tone rather than the balance.")
        color: "#aaa"
        font.italic: true
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.topMargin: 10
    }
}
