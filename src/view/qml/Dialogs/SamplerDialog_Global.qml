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
    Layout.fillWidth: true
    Layout.preferredWidth: parent.width * 0.33
    Layout.alignment: Qt.AlignTop
    spacing: 15

    // Pan Knob
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: samplerController.selectedPadPan * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadPan = v / Constants.uiInternalScaling;
        }
    }

    // Volume Knob
    Knob {
        label: qsTr("Volume")
        mapping: "volume"
        value: samplerController.selectedPadVolume * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadVolume = v / Constants.uiInternalScaling;
        }
    }

    // LPF Cutoff Knob
    FilterKnob {
        label: qsTr("LPF Cutoff")
        controller: samplerController
        value: samplerController.selectedPadCutoff * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadCutoff = v / Constants.uiInternalScaling;
        }
    }

    // HPF Cutoff Knob
    FilterKnob {
        label: qsTr("HPF Cutoff")
        controller: samplerController
        value: samplerController.selectedPadHpfCutoff * Constants.uiInternalScaling
        isHpf: true
        onMoved: v => {
            samplerController.selectedPadHpfCutoff = v / Constants.uiInternalScaling;
        }
    }

    // Global Settings
    Label {
        text: qsTr("Global Settings")
        font.bold: true
        color: themeService.accentColor
        Layout.topMargin: 10
    }

    Knob {
        label: qsTr("Global Volume")
        mapping: "volume"
        value: samplerController.volume
        onMoved: v => {
            samplerController.volume = v;
        }
    }

    Knob {
        label: qsTr("Global Gain")
        mapping: "decibel"
        mapMin: -30
        mapMax: 30
        value: samplerController.gain
        onMoved: v => {
            samplerController.gain = v;
        }
    }

    SamplerDialog_Offset {
        Layout.fillWidth: true
    }

    CheckBox {
        id: channelModeCheckbox
        text: qsTr("Map pads to MIDI channels 1-16 (for MIDI CC automation only)")
        Layout.alignment: Qt.AlignLeft
        Layout.leftMargin: 20
        checked: samplerController.channelMode
        onToggled: samplerController.channelMode = checked
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("When enabled, MIDI CCs on channel 1 affect pad 1, channel 2 affects pad 2, and so on. Note playing is unaffected.")
    }
}
