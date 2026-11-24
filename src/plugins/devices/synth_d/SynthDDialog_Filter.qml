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

    SectionHeading {
        text: qsTr("Filter")
        Layout.topMargin: 10
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("Low-Pass"), qsTr("High-Pass")];
            }
            currentIndex: synthDController.filterMode
            onActivated: i => synthDController.filterMode = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Cutoff Frequency")
        mapping: "exponentialHz"
        suffix: "Hz"
        mapMin: 10
        mapMax: 20000
        value: synthDController.cutoff
        onMoved: v => synthDController.cutoff = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Where the filter starts to cut. The panel's -4 to +4 runs from 10 Hz to 20 kHz, with 0 at 447 Hz and each step about 1.4 octaves.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Emphasis")
        mapping: "value"
        suffix: ""
        mapMin: 0
        mapMax: 10
        value: synthDController.emphasis
        onMoved: v => synthDController.emphasis = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Resonance at the cutoff. From about 8 the filter rings on its own.")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Amount of Contour")
        mapping: "value"
        suffix: ""
        mapMin: 0
        mapMax: 10
        value: synthDController.contourAmount
        onMoved: v => synthDController.contourAmount = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How far the filter contour opens the cutoff above where the knob sets it: up to 4 octaves at 10")
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Keyboard Control 1")
        checked: synthDController.keyboardControl1
        onToggled: synthDController.keyboardControl1 = checked
        ToolTip.visible: hovered
        ToolTip.text: qsTr("A third of the keyboard's pitch moves the cutoff; with 2 as well, all of it")
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Keyboard Control 2")
        checked: synthDController.keyboardControl2
        onToggled: synthDController.keyboardControl2 = checked
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Two thirds of the keyboard's pitch moves the cutoff; with 1 as well, all of it")
        Layout.fillWidth: true
    }
}
