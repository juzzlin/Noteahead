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
        text: qsTr("Filter")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }


    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: synthController
        value: synthController.lpfCutoff
        onMoved: v => synthController.lpfCutoff = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("LPF Resonance")
        value: synthController.lpfResonance
        onMoved: v => synthController.lpfResonance = v
        Layout.fillWidth: true
    }
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("LPF Slope")
        }
        ComboBox {
            model: ["12 dB/oct", "24 dB/oct"]
            currentIndex: synthController.lpfSlope
            onActivated: idx => synthController.lpfSlope = idx
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How steeply the low pass rolls off. The gentler one keeps the top of a sound where the steeper one takes it away.")
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("LPF Key Track")
        value: synthController.filterKeyTrack
        onMoved: v => synthController.filterKeyTrack = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the note played moves the LPF cutoff. The high pass does not follow the keyboard.")
        Layout.fillWidth: true
    }
    LayoutSeparator {}

    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: synthController
        value: synthController.hpfCutoff
        isHpf: true
        onMoved: v => synthController.hpfCutoff = v
        Layout.fillWidth: true
    }
    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("HPF Slope")
        }
        ComboBox {
            model: ["12 dB/oct", "24 dB/oct"]
            currentIndex: synthController.hpfSlope
            onActivated: idx => synthController.hpfSlope = idx
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How steeply the high pass rolls off. The steeper one clears more out of the way at the same cutoff.")
            Layout.fillWidth: true
        }
    }
}
