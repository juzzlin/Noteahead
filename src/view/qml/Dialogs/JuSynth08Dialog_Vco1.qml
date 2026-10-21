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
        text: qsTr("VCO 1")
        Layout.topMargin: 10
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Range")
        }
        ComboBox {
            model: ["16'", "8'", "4'", "2'"]
            currentIndex: juSynth08Controller.vco1Range
            onActivated: i => juSynth08Controller.vco1Range = i
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Waveform")
        }
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("Triangle"), qsTr("Saw"), qsTr("Square"), qsTr("Sine")];
            }
            currentIndex: juSynth08Controller.vco1Waveform
            onActivated: i => juSynth08Controller.vco1Waveform = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Level")
        value: juSynth08Controller.vco1Level
        onMoved: v => juSynth08Controller.vco1Level = v
        Layout.fillWidth: true
    }
}
