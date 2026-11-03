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
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import Noteahead 1.0
import "../Components"

//! One LFO of the LFO device: what it moves, and how. The dialog repeats this over lfoController.lfos.
ColumnLayout {
    id: root

    //! The LfoPanelController this panel edits.
    required property var lfo
    //! Everything a parameter automation can drive, as propertyService.automationTargets() lists it.
    required property var targets

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop
    spacing: 6

    RowLayout {
        SectionHeading {
            text: "LFO " + (root.lfo.index + 1)
        }
        LfoLed {
            controller: lfoController
            lfoIndex: root.lfo.index
        }
    }

    ModulationTargetPicker {
        targets: root.targets
        owner: root.lfo.targetOwner
        ownerId: root.lfo.targetOwnerId
        parameter: root.lfo.targetParameter
        shared: root.lfo.targetShared
        onTargetChosen: (owner, ownerId, parameter) => root.lfo.setTarget(owner, ownerId, parameter)
        onTargetCleared: root.lfo.clearTarget()
    }

    RowLayout {
        Layout.fillWidth: true
        ComboBox {
            model: lfoController.waveformNames
            currentIndex: root.lfo.waveform
            onActivated: index => root.lfo.waveform = index
            Layout.fillWidth: true
        }
        ComboBox {
            model: [qsTr("Free"), qsTr("Sync"), qsTr("One-shot")]
            currentIndex: root.lfo.mode
            onActivated: index => root.lfo.mode = index
            Layout.fillWidth: true
        }
        ComboBox {
            model: [qsTr("Bipolar"), qsTr("Unipolar"), qsTr("Tremolo")]
            currentIndex: root.lfo.modulationMode
            onActivated: index => root.lfo.modulationMode = index
            Layout.fillWidth: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Bipolar swings either way of the setting. Unipolar moves from the setting towards the control's rest. Tremolo scales a fader's gain, like Expression.")
        }
    }

    Knob {
        label: qsTr("Rate")
        mapping: "lfoFrequency"
        mapMin: 0.05
        mapMax: 19.05
        suffix: "Hz"
        value: root.lfo.rate
        onMoved: value => root.lfo.rate = value
        Layout.fillWidth: true
        visible: root.lfo.mode !== 1
    }
    SyncSlider {
        label: qsTr("Rate")
        value: root.lfo.rate
        onMoved: value => root.lfo.rate = value
        Layout.fillWidth: true
        visible: root.lfo.mode === 1
    }

    GridLayout {
        columns: 2
        columnSpacing: 10
        rowSpacing: 6
        Layout.fillWidth: true

        Knob {
            label: qsTr("Rate Divider")
            mapping: "integer"
            suffix: ""
            from: 1
            to: lfoController.maxRateDivider
            stepSize: 1
            value: root.lfo.rateDivider
            onMoved: value => root.lfo.rateDivider = Math.round(value)
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Divides the Rate by up to %1, in both modes: a tempo-locked LFO stretched over several bars, a free-running one over minutes.").arg(lfoController.maxRateDivider)
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Depth")
            // Bipolar swings half the depth either way of the setting, so it reads as that swing
            mapping: root.lfo.modulationMode === 0 ? "swing" : "linear"
            mapMax: root.lfo.modulationMode === 0 ? 50 : to
            value: root.lfo.depth
            onMoved: value => root.lfo.depth = value
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Phase")
            mapping: "value"
            mapMin: 0
            mapMax: 360
            suffix: "°"
            value: root.lfo.phase
            onMoved: value => root.lfo.phase = value
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Smoothing")
            mapping: "cubic"
            mapMin: 0
            mapMax: 250
            suffix: "ms"
            value: root.lfo.smoothing
            onMoved: value => root.lfo.smoothing = value
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Rounds off the steps of Square and Random, which otherwise click.")
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Delay")
            mapping: "cubic"
            mapMin: 0
            mapMax: Constants.maxLfoEnvelopeTimeMs
            suffix: "ms"
            value: root.lfo.delay
            onMoved: value => root.lfo.delay = value
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Fade")
            mapping: "cubic"
            mapMin: 0
            mapMax: Constants.maxLfoEnvelopeTimeMs
            suffix: "ms"
            value: root.lfo.fade
            onMoved: value => root.lfo.fade = value
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Velocity")
            value: root.lfo.velocitySensitivity
            onMoved: value => root.lfo.velocitySensitivity = value
            ToolTip.visible: hovered
            ToolTip.text: qsTr("How much a softer note on this device's track lessens the depth.")
            Layout.fillWidth: true
        }
        CheckBox {
            text: qsTr("Restart on note")
            checked: root.lfo.retrigger
            onToggled: root.lfo.retrigger = checked
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Notes on this device's track start the LFO over. One-shot always does.")
        }
    }
}
