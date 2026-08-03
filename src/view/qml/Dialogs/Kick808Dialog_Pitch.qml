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
        text: qsTr("Pitch")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Pitch Depth")
        value: kick808Controller.pitchDepth
        onMoved: v => kick808Controller.pitchDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Pitch Decay")
        value: kick808Controller.pitchDecay
        onMoved: v => kick808Controller.pitchDecay = v
        Layout.fillWidth: true
    }

    CheckBox {
        text: qsTr("Key Track")
        checked: kick808Controller.keyTrack
        onToggled: kick808Controller.keyTrack = checked
        Layout.fillWidth: true
        Layout.topMargin: 5
    }

    Label {
        text: qsTr("On, the pitch follows the note column and Glide slurs between notes. Off, it stays at a fixed C2. Tuning offsets either by up to two octaves.")
        font.pixelSize: 11
        color: "#999"
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        // Wrapping text must not push the column wider than its siblings, or the whole dialog
        // reflows whenever the text changes.
        Layout.preferredWidth: 0
    }

    Knob {
        label: qsTr("Glide")
        value: kick808Controller.glide
        onMoved: v => kick808Controller.glide = v
        enabled: kick808Controller.keyTrack
        Layout.fillWidth: true
    }
}
