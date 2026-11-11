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

// The modulation bus: two sources mixed, the depth the mod wheel turns, and where it goes
ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    SectionHeading {
        text: qsTr("Modulation")
        Layout.topMargin: 10
    }
    // The two ends of the Mod Mix knob, each a switch on the panel
    ComboBox {
        model: {
            languageService.activeLanguage;
            return [qsTr("OSC 3"), qsTr("Filter EG")];
        }
        currentIndex: synthDController.modSource1
        onActivated: i => synthDController.modSource1 = i
        ToolTip.visible: hovered
        ToolTip.text: qsTr("The source at the left end of Mod Mix: OSC 3 as a slow or audio-rate modulator, or the filter contour")
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
    }
    ComboBox {
        model: {
            languageService.activeLanguage;
            return [qsTr("Noise"), qsTr("LFO")];
        }
        currentIndex: synthDController.modSource2
        onActivated: i => synthDController.modSource2 = i
        ToolTip.visible: hovered
        ToolTip.text: qsTr("The source at the right end of Mod Mix: noise for random wobble, or the LFO")
        Layout.fillWidth: true
        Layout.bottomMargin: Constants.dropDownBottomMargin
    }
    Knob {
        label: qsTr("Mod Mix")
        mapping: "value"
        suffix: ""
        mapMin: 0
        mapMax: 10
        value: synthDController.modMix
        onMoved: v => synthDController.modMix = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Blends the two sources: OSC 3 or Filter EG at 0, Noise or LFO at 10")
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Mod Depth")
        mapping: "value"
        suffix: ""
        mapMin: 0
        mapMax: 10
        value: synthDController.modDepth
        onMoved: v => synthDController.modDepth = v
        ToolTip.visible: hovered
        ToolTip.text: qsTr("How much modulation reaches its destinations. The modulation wheel turns this knob.")
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Oscillator Modulation")
        checked: synthDController.oscillatorModulation
        onToggled: synthDController.oscillatorModulation = checked
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Sends the modulation to the pitch of the VCOs, for vibrato and trills. OSC 3 is left out while it is off the keyboard.")
        Layout.fillWidth: true
    }
    CheckBox {
        text: qsTr("Filter Modulation")
        checked: synthDController.filterModulation
        onToggled: synthDController.filterModulation = checked
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Sends the modulation to the filter cutoff, for wah and growl")
        Layout.fillWidth: true
    }
}
