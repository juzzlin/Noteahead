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
    Layout.alignment: Qt.AlignTop
    spacing: 15

    Label {
        text: qsTr("Pad Settings")
        font.bold: true
        color: themeService.accentColor
    }

    // Pan Knob
    Knob {
        label: qsTr("Pan")
        mapping: "pan"
        value: samplerController.selectedPadPan * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadPan = v / Constants.uiInternalScaling;
        }
    }

    // Fader Knob
    Knob {
        label: qsTr("Fader")
        mapping: "fader"
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

    Knob {
        label: qsTr("Tune")
        mapping: "bipolar"
        mapMin: -24
        mapMax: 24
        suffix: " st"
        value: samplerController.selectedPadTune * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadTune = v / Constants.uiInternalScaling;
        }
    }

    Knob {
        label: qsTr("Fine Tune")
        mapping: "bipolar"
        mapMin: -100
        mapMax: 100
        suffix: " ct"
        value: samplerController.selectedPadDetune * Constants.uiInternalScaling
        onMoved: v => {
            samplerController.selectedPadDetune = v / Constants.uiInternalScaling;
        }
    }

    SamplerDialog_Offsets {
        Layout.fillWidth: true
    }

    CheckBox {
        id: reverseCheckBox
        text: qsTr("Reverse")
        Layout.fillWidth: true
        checked: samplerController.selectedPadReverse
        onToggled: samplerController.selectedPadReverse = checked
        contentItem: Label {
            text: reverseCheckBox.text
            color: "white"
            verticalAlignment: Text.AlignVCenter
            leftPadding: reverseCheckBox.indicator.width + reverseCheckBox.spacing
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Plays the pad backwards. The offsets follow the reversed waveform.")
    }

    CheckBox {
        id: loopCheckBox
        text: qsTr("Loop")
        Layout.fillWidth: true
        checked: samplerController.selectedPadLoop
        onToggled: samplerController.selectedPadLoop = checked
        contentItem: Label {
            text: loopCheckBox.text
            color: "white"
            verticalAlignment: Text.AlignVCenter
            leftPadding: loopCheckBox.indicator.width + loopCheckBox.spacing
        }
        ToolTip.delay: Constants.toolTipDelay
        ToolTip.timeout: Constants.toolTipTimeout
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Repeats the pad's range until the note is released. The amp envelope decides when a looping pad falls silent.")
    }

    ColumnLayout {
        Layout.fillWidth: true
        Label {
            text: qsTr("Choke Group:")
            color: "white"
        }
        SpinBox {
            id: chokeGroupSpinBox
            Layout.fillWidth: true
            from: 0
            to: 8
            value: samplerController.selectedPadChokeGroup
            editable: true
            onValueModified: samplerController.selectedPadChokeGroup = value
            Keys.onReturnPressed: {
                value = valueFromText(contentItem.text, locale);
                samplerController.selectedPadChokeGroup = value;
            }
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Triggering this pad silences the other pads sharing its group, the way a closed hi-hat cuts an open one. Zero puts the pad in no group.")
        }
    }
}
