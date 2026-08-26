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
        text: qsTr("Ensemble Chorus")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 5
    }

    CheckBox {
        text: qsTr("Enable Ensemble")
        checked: stringVoiceV2Controller.ensembleEnabled
        onToggled: stringVoiceV2Controller.ensembleEnabled = checked
        Layout.fillWidth: true
    }

    RowLayout {
        Layout.fillWidth: true
        enabled: stringVoiceV2Controller.ensembleEnabled
        spacing: 10

        Label {
            text: qsTr("Mode:")
        }

        ComboBox {
            id: modeCombo
            model: [qsTr("Chorus I"), qsTr("Chorus II"), qsTr("Chorus I + II")]
            currentIndex: stringVoiceV2Controller.ensembleMode
            onActivated: stringVoiceV2Controller.ensembleMode = currentIndex
            Layout.fillWidth: true
        }
    }

    Label {
        text: qsTr("Vibrato")
        font.bold: true
        font.pixelSize: 14
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Rate")
        value: stringVoiceV2Controller.vibratoRate
        onMoved: v => stringVoiceV2Controller.vibratoRate = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Depth")
        value: stringVoiceV2Controller.vibratoDepth
        onMoved: v => stringVoiceV2Controller.vibratoDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Delay")
        value: stringVoiceV2Controller.vibratoDelay
        onMoved: v => stringVoiceV2Controller.vibratoDelay = v
        Layout.fillWidth: true
    }
}
