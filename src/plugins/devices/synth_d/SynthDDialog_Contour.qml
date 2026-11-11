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

// The filter's or the loudness contour: attack, decay and sustain, and the Decay switch that lets
// the note die away over the decay time once the key is up
ColumnLayout {
    id: root

    property bool loudness: false
    readonly property string prefix: loudness ? "loudness" : "filter"

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: root.loudness ? qsTr("Loudness Contour") : qsTr("Filter Contour")
        Layout.topMargin: 10
    }
    Knob {
        label: qsTr("Attack Time")
        mapping: "value"
        suffix: ""
        mapMin: 0
        mapMax: 10
        value: synthDController[root.prefix + "Attack"]
        onMoved: v => synthDController[root.prefix + "Attack"] = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("1 ms at 0, 10 ms at 0.5, 200 ms at 2, 600 ms at 4, 1 s at 6, 5 s at 8 and 10 s at 10")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Decay Time")
        mapping: "value"
        suffix: ""
        mapMin: 0
        mapMax: 10
        value: synthDController[root.prefix + "Decay"]
        onMoved: v => synthDController[root.prefix + "Decay"] = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("4 ms at 0, 10 ms at 0.5, 200 ms at 2, 600 ms at 4, 1 s at 6, 5 s at 8 and 35 s at 10")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sustain Level")
        mapping: "value"
        suffix: ""
        mapMin: 0
        mapMax: 10
        value: synthDController[root.prefix + "Sustain"]
        onMoved: v => synthDController[root.prefix + "Sustain"] = v
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Decay")
        checked: synthDController[root.prefix + "DecaySwitch"]
        onToggled: synthDController[root.prefix + "DecaySwitch"] = checked
        ToolTip.visible: hovered
        ToolTip.text: qsTr("On, a released note dies away over the decay time. Off, it stops at once.")
        Layout.fillWidth: true
    }
}
