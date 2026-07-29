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
    id: root
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 12

    Label {
        text: qsTr("Global")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 5
    }

    Knob {
        label: qsTr("Master Volume")
        mapping: "volume"
        value: stringEnsembleController.volume
        onMoved: v => stringEnsembleController.volume = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: stringEnsembleController.gain
        onMoved: v => stringEnsembleController.gain = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: stringEnsembleController.pan
        onMoved: v => stringEnsembleController.pan = v
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("Filter")
        font.bold: true
        font.pixelSize: 14
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: stringEnsembleController
        value: stringEnsembleController.lpfCutoff
        onMoved: v => stringEnsembleController.lpfCutoff = v
        Layout.fillWidth: true
    }

    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: stringEnsembleController
        isHpf: true
        value: stringEnsembleController.hpfCutoff
        onMoved: v => stringEnsembleController.hpfCutoff = v
        Layout.fillWidth: true
    }
}
