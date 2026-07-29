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
        text: qsTr("Bass Section")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 5
    }

    Label {
        text: qsTr("Played by keys below B3")
        font.pixelSize: 12
        opacity: 0.7
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    CheckBox {
        text: qsTr("Contrabass 16'")
        checked: stringEnsembleController.contrabassEnabled
        onToggled: stringEnsembleController.contrabassEnabled = checked
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Cello 8'")
        checked: stringEnsembleController.celloEnabled
        onToggled: stringEnsembleController.celloEnabled = checked
        Layout.fillWidth: true
    }

    Knob {
        label: qsTr("Volume Bass")
        value: stringEnsembleController.volumeBass
        onMoved: v => stringEnsembleController.volumeBass = v
        Layout.fillWidth: true
    }

    Label {
        text: qsTr("Upper Section")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 15
    }

    Label {
        text: qsTr("Played by keys from B3 up")
        font.pixelSize: 12
        opacity: 0.7
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
    }

    CheckBox {
        text: qsTr("Horn 16'")
        checked: stringEnsembleController.hornEnabled
        onToggled: stringEnsembleController.hornEnabled = checked
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Viola 8'")
        checked: stringEnsembleController.violaEnabled
        onToggled: stringEnsembleController.violaEnabled = checked
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Trumpet 8'")
        checked: stringEnsembleController.trumpetEnabled
        onToggled: stringEnsembleController.trumpetEnabled = checked
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Violin 4'")
        checked: stringEnsembleController.violinEnabled
        onToggled: stringEnsembleController.violinEnabled = checked
        Layout.fillWidth: true
    }
}
