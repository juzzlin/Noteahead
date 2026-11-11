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
        text: qsTr("Tone")
        Layout.topMargin: 10
    }
    ColumnLayout {
        spacing: 2
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Waveform")
        }
        ComboBox {
            // In PolyBlepOscillator::Waveform order
            model: {
                languageService.activeLanguage;
                return [qsTr("Triangle"), qsTr("Saw"), qsTr("Square"), qsTr("Sine")];
            }
            currentIndex: noiseFxController.toneWaveform
            onActivated: i => noiseFxController.toneWaveform = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Pitch")
        mapping: "bipolar"
        mapMin: -24
        mapMax: 24
        suffix: " st"
        value: noiseFxController.pitch
        onMoved: v => noiseFxController.pitch = v
        Layout.fillWidth: true
    }
    Knob {
        // All noise at the bottom, all tone at the top
        label: qsTr("Blend")
        value: noiseFxController.blend
        onMoved: v => noiseFxController.blend = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Ring Mod")
        value: noiseFxController.ringMod
        onMoved: v => noiseFxController.ringMod = v
        Layout.fillWidth: true
    }
}
