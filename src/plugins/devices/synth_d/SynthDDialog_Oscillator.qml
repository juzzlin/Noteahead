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

// One VCO of the bank. OSC 1 has no frequency knob of its own, and OSC 3 can leave the keyboard
// to be a modulation source; its second waveform is a reverse saw rather than a triangle-saw.
ColumnLayout {
    id: root

    property int number: 1
    readonly property string prefix: "osc" + number

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("OSC %1").arg(root.number)
        Layout.topMargin: 10
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        ComboBox {
            model: ["LO", "32'", "16'", "8'", "4'", "2'"]
            currentIndex: synthDController[root.prefix + "Range"]
            onActivated: i => synthDController[root.prefix + "Range"] = i
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("Triangle"), root.number === 3 ? qsTr("Reverse Saw") : qsTr("Triangle-Saw"), qsTr("Saw"), qsTr("Square"), qsTr("Wide Pulse"), qsTr("Narrow Pulse")];
            }
            currentIndex: synthDController[root.prefix + "Waveform"]
            onActivated: i => synthDController[root.prefix + "Waveform"] = i
            Layout.fillWidth: true
        }
    }
    Knob {
        visible: root.number > 1
        label: qsTr("Frequency")
        mapping: "value"
        suffix: ""
        mapMin: -7
        mapMax: 7
        value: root.number > 1 ? synthDController[root.prefix + "Frequency"] : 0
        onMoved: v => synthDController[root.prefix + "Frequency"] = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Semitones from OSC 1")
        Layout.fillWidth: true
    }
    CheckBox {
        visible: root.number === 3
        text: qsTr("Keyboard Control")
        checked: synthDController.osc3KeyboardControl
        onToggled: synthDController.osc3KeyboardControl = checked
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Off, OSC 3 ignores the keys and the wheels and stays where its knobs put it")
        Layout.fillWidth: true
    }
}
