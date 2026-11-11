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
        text: qsTr("VCO Mod")
        Layout.topMargin: 10
    }
    Knob {
        label: qsTr("LFO Mod")
        value: juSynth08Controller.pitchLfoDepth
        onMoved: v => juSynth08Controller.pitchLfoDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Env Mod")
        mapMin: -24
        mapMax: 24
        suffix: "st"
        value: juSynth08Controller.pitchEnvDepth
        onMoved: v => juSynth08Controller.pitchEnvDepth = v
        Layout.fillWidth: true
    }
    // Label over the box: beside it, a quarter of the row leaves the box too narrow for its text
    Label {
        text: qsTr("Env Target")
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("VCO 1"), qsTr("VCO 2"), qsTr("Both")];
            }
            currentIndex: juSynth08Controller.pitchEnvTarget
            onActivated: i => juSynth08Controller.pitchEnvTarget = i
            Layout.fillWidth: true
        }
    }
    Label {
        text: qsTr("PWM Source")
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("Manual"), qsTr("LFO"), qsTr("ENV-1")];
            }
            currentIndex: juSynth08Controller.pwmSource
            onActivated: i => juSynth08Controller.pwmSource = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Pulse Width")
        value: juSynth08Controller.pulseWidth
        onMoved: v => juSynth08Controller.pulseWidth = v
        Layout.fillWidth: true
    }
}
