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
        Layout.fillWidth: true
        Layout.topMargin: 10
        SectionHeading {
            text: qsTr("LFO 2")
        }
        LfoLed {
            controller: synthController
            lfoIndex: 1
        }
        Item {
            Layout.fillWidth: true
        }
        CheckBox {
            text: qsTr("Free Run")
            checked: synthController.lfo2FreeRun
            onToggled: synthController.lfo2FreeRun = checked
            ToolTip.visible: hovered
            ToolTip.text: qsTr("One LFO for all the voices that runs on regardless of the notes. Its Delay and Fade restart on the first note after silence. Off, every note restarts its own LFO.")
        }
    }

    RowLayout {
        ComboBox {
            model: synthController.lfo2WaveformNames
            currentIndex: synthController.lfo2Waveform
            onActivated: i => synthController.lfo2Waveform = i
            Layout.fillWidth: true
        }
        ComboBox {
            model: synthController.lfo2ModeNames
            currentIndex: synthController.lfo2Mode
            onActivated: i => synthController.lfo2Mode = i
            Layout.fillWidth: true
        }
    }
    RowLayout {
        ComboBox {
            model: synthController.lfo2TargetNames
            currentIndex: synthController.lfo2Target
            onActivated: i => synthController.lfo2Target = i
            Layout.fillWidth: true
        }
    }
    Knob {
        label: qsTr("Rate")
        value: synthController.lfo2Rate
        onMoved: v => synthController.lfo2Rate = v
        Layout.fillWidth: true
        visible: synthController.lfo2Mode !== 1
    }
    SyncSlider {
        label: qsTr("Rate")
        value: synthController.lfo2Rate
        onMoved: v => synthController.lfo2Rate = v
        Layout.fillWidth: true
        visible: synthController.lfo2Mode === 1
    }
    Knob {
        label: qsTr("Rate Divider")
        mapping: "integer"
        suffix: ""
        from: 1
        to: synthController.maxLfoRateDivider
        stepSize: 1
        value: synthController.lfo2RateDivider
        onMoved: v => synthController.lfo2RateDivider = Math.round(v)
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Divides the Rate by up to %1, in both modes: a tempo-locked LFO stretched over several bars, a free-running one over minutes.").arg(synthController.maxLfoRateDivider)
        Layout.fillWidth: true
    }
    Knob {
        label: qsTr("Intensity")
        mapping: "cubicCentered"
        mapMin: -100
        mapMax: 100
        value: synthController.lfo2Int
        onMoved: v => synthController.lfo2Int = v
        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Delay")
            mapping: "cubic"
            mapMin: 0
            mapMax: Constants.maxLfoEnvelopeTimeMs
            suffix: "ms"
            value: synthController.lfo2Delay
            onMoved: v => synthController.lfo2Delay = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Fade")
            mapping: "cubic"
            mapMin: 0
            mapMax: Constants.maxLfoEnvelopeTimeMs
            suffix: "ms"
            value: synthController.lfo2Fade
            onMoved: v => synthController.lfo2Fade = v
            Layout.fillWidth: true
        }
    }
}
