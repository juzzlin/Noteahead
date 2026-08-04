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
        text: qsTr("Voice Section")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 5
    }

    Label {
        text: qsTr("Lower (below C4)")
        font.bold: true
        font.pixelSize: 13
        color: "#aaa"
        Layout.alignment: Qt.AlignLeft
    }
    Knob {
        label: qsTr("Male 8'")
        value: stringVoiceController.voiceMale8
        onMoved: v => stringVoiceController.voiceMale8 = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Male 4'")
        value: stringVoiceController.voiceMale4
        onMoved: v => stringVoiceController.voiceMale4 = v
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("Upper (C4 and above)")
        font.bold: true
        font.pixelSize: 13
        color: "#aaa"
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 6
    }
    Knob {
        label: qsTr("Male 8'")
        value: stringVoiceController.voiceUpperMale8
        onMoved: v => stringVoiceController.voiceUpperMale8 = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Female 4'")
        value: stringVoiceController.voiceFemale4
        onMoved: v => stringVoiceController.voiceFemale4 = v
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("The female voice sounds only above the split, an octave up, as on the hardware.")
        color: "#aaa"
        font.italic: true
        font.pixelSize: 11
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Attack Time")
        value: stringVoiceController.voiceAttack
        onMoved: v => stringVoiceController.voiceAttack = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Release Time")
        value: stringVoiceController.voiceRelease
        onMoved: v => stringVoiceController.voiceRelease = v
        Layout.fillWidth: true
    }
}
