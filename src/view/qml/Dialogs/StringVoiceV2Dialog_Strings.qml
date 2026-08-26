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
        text: qsTr("Strings Section")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 5
    }

    CheckBox {
        text: qsTr("Upper")
        checked: stringVoiceV2Controller.stringsUpper
        onToggled: stringVoiceV2Controller.stringsUpper = checked
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Lower")
        checked: stringVoiceV2Controller.stringsLower
        onToggled: stringVoiceV2Controller.stringsLower = checked
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Tone")
        value: stringVoiceV2Controller.stringsTone
        onMoved: v => stringVoiceV2Controller.stringsTone = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Attack Time")
        value: stringVoiceV2Controller.stringsAttack
        onMoved: v => stringVoiceV2Controller.stringsAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release Time")
        value: stringVoiceV2Controller.stringsRelease
        onMoved: v => stringVoiceV2Controller.stringsRelease = v
        Layout.fillWidth: true
    }
}
