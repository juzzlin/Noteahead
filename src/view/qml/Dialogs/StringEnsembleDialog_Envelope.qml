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
    spacing: 12

    Label {
        text: qsTr("Envelope")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 5
    }

    Knob {
        label: qsTr("Crescendo")
        value: stringEnsembleController.crescendo
        onMoved: v => stringEnsembleController.crescendo = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain Length")
        value: stringEnsembleController.sustainLength
        onMoved: v => stringEnsembleController.sustainLength = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Velocity")
        value: stringEnsembleController.velocitySensitivity
        onMoved: v => stringEnsembleController.velocitySensitivity = v
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("At zero every note plays at full level")
        font.pixelSize: 12
        opacity: 0.7
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    Label {
        text: qsTr("Modulation")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 15
    }

    CheckBox {
        text: qsTr("Ensemble")
        checked: stringEnsembleController.modulationEnabled
        onToggled: stringEnsembleController.modulationEnabled = checked
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("Phaser")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 15
    }

    CheckBox {
        text: qsTr("Enable Phaser")
        checked: stringEnsembleController.phaserEnabled
        onToggled: stringEnsembleController.phaserEnabled = checked
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Color")
        enabled: stringEnsembleController.phaserEnabled
        value: stringEnsembleController.phaserColor
        onMoved: v => stringEnsembleController.phaserColor = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Rate")
        enabled: stringEnsembleController.phaserEnabled
        value: stringEnsembleController.phaserRate
        onMoved: v => stringEnsembleController.phaserRate = v
        Layout.fillWidth: true
    }
}
