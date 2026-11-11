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
        text: qsTr("DCO")
        Layout.topMargin: 10
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("Range")
        }
        ComboBox {
            // Footages, the way the panel marks them
            model: ["16'", "8'", "4'"]
            currentIndex: juSynthController.range
            onActivated: i => juSynthController.range = i
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        CheckBox {
            text: qsTr("Saw")
            checked: juSynthController.sawEnabled
            onToggled: juSynthController.sawEnabled = checked
            Layout.fillWidth: true
        }
        CheckBox {
            text: qsTr("Pulse")
            checked: juSynthController.pulseEnabled
            onToggled: juSynthController.pulseEnabled = checked
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
        Label {
            text: qsTr("PWM Source")
        }
        ComboBox {
            model: {
                languageService.activeLanguage;
                return [qsTr("LFO"), qsTr("Manual"), qsTr("Env")];
            }
            currentIndex: juSynthController.pwmSource
            onActivated: i => juSynthController.pwmSource = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("PWM")
        enabled: juSynthController.pulseEnabled
        value: juSynthController.pulseWidth
        onMoved: v => juSynthController.pulseWidth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("LFO Mod")
        value: juSynthController.pitchLfoDepth
        onMoved: v => juSynthController.pitchLfoDepth = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Sub Level")
        value: juSynthController.subLevel
        onMoved: v => juSynthController.subLevel = v
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Noise Level")
        value: juSynthController.noiseLevel
        onMoved: v => juSynthController.noiseLevel = v
        Layout.fillWidth: true
    }
}
