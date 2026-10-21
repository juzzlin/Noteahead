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

    RowLayout {
        Layout.topMargin: 10
        SectionHeading {
            text: qsTr("LFO")
        }
        LfoLed {
            controller: juSynth08Controller
            lfoIndex: 0
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
                return [qsTr("Sine"), qsTr("Triangle"), qsTr("Saw"), qsTr("Square"), qsTr("S&H"), qsTr("Noise")];
            }
            currentIndex: juSynth08Controller.lfoWaveform
            onActivated: i => juSynth08Controller.lfoWaveform = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Rate")
        suffix: "Hz"
        mapping: "exponential"
        mapMin: 0.1
        mapMax: 30.0
        value: juSynth08Controller.lfoRate
        onMoved: v => juSynth08Controller.lfoRate = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Delay")
        suffix: "ms"
        mapping: "linear"
        mapMin: 0
        mapMax: 3000
        value: juSynth08Controller.lfoDelay
        onMoved: v => juSynth08Controller.lfoDelay = v
        Layout.fillWidth: true
    }
}
