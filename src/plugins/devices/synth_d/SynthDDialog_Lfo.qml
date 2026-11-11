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

// The Behringer's own LFO, one of the modulation sources
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    RowLayout {
        Layout.topMargin: 10
        SectionHeading {
            text: qsTr("LFO")
        }
        LfoLed {
            controller: synthDController
            lfoIndex: 0
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("Triangle"), qsTr("Square")];
            }
            currentIndex: synthDController.lfoWaveform
            onActivated: i => synthDController.lfoWaveform = i
            ToolTip.visible: hovered
            ToolTip.text: qsTr("The LFO's shape: a smooth triangle, or a square that jumps between two values")
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Rate")
        suffix: "Hz"
        mapping: "exponential"
        mapMin: 0.05
        mapMax: 200.0
        value: synthDController.lfoRate
        onMoved: v => synthDController.lfoRate = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How fast the LFO runs. The LED follows it. Choose LFO as the second modulation source to hear it.")
        Layout.fillWidth: true
    }
}
