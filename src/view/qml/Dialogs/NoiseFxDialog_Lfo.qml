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
        text: qsTr("LFO")
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
            // In Lfo::Waveform order, and then NoiseFxDevice::SmoothRandomWaveform
            model: {
                languageService.activeLanguage;
                return [qsTr("Saw"), qsTr("Triangle"), qsTr("Square"), qsTr("Sine"), qsTr("Random"), qsTr("Smooth Random")];
            }
            currentIndex: noiseFxController.lfoWaveform
            onActivated: i => noiseFxController.lfoWaveform = i
            Layout.fillWidth: true
        }
    }
    CheckBox {
        text: qsTr("Sync")
        checked: noiseFxController.lfoSync
        onToggled: noiseFxController.lfoSync = checked
    }
    ColumnLayout {
        spacing: 2
        visible: noiseFxController.lfoSync
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Rate")
        }
        ComboBox {
            model: noiseFxController.lfoSyncNames
            currentIndex: noiseFxController.lfoSyncDivision
            onActivated: i => noiseFxController.lfoSyncDivision = i
            Layout.fillWidth: true
        }
    }
    Knob {
        visible: !noiseFxController.lfoSync
        label: qsTr("Rate")
        suffix: "Hz"
        mapping: "exponential"
        mapMin: 0.05
        mapMax: 50
        value: noiseFxController.lfoRate
        onMoved: v => noiseFxController.lfoRate = v
        Layout.fillWidth: true
    }
    NoiseFxDialog_Route {
        isLfo: true
        target1: noiseFxController.lfoTarget1
        target2: noiseFxController.lfoTarget2
        depth1: noiseFxController.lfoDepth1
        depth2: noiseFxController.lfoDepth2
        onTarget1Activated: target => noiseFxController.lfoTarget1 = target
        onTarget2Activated: target => noiseFxController.lfoTarget2 = target
        onDepth1Moved: depth => noiseFxController.lfoDepth1 = depth
        onDepth2Moved: depth => noiseFxController.lfoDepth2 = depth
    }
}
