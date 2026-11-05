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


EffectDialog {
    id: root
    title: "<strong>" + qsTr("Reverb 224 (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: parent ? parent.width * Constants.largeDialogScale : 800
    height: parent ? parent.height * Constants.largeDialogScale : 600

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: Theme.surfaceDialog
        border.color: Theme.borderSubtle
        radius: 2
    }

    readonly property int program: {
        effectRackController.revision;
        return Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.reverb224ProgramKey()));
    }

    //! A knob on one of the effect's parameters, which run 0..1 and are shown over 0..scale.
    component ParameterKnob: Knob {
        property string key
        property real scale: 100
        from: 0
        to: scale
        value: {
            effectRackController.revision;
            return effectRackController.parameterValue(root.effectIndex, key) * scale;
        }
        onMoved: v => effectRackController.setParameterValue(root.effectIndex, key, v / scale)
        Layout.fillWidth: true
    }

    component SectionLabel: Label {
        font.bold: true
        color: "#ccc"
        Layout.topMargin: 4
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 12

            EffectPresetRow {
                effectIndex: root.effectIndex
                showWhenEmpty: true
            }

            Label {
                text: qsTr("A late-seventies digital reverb: an eight-line tank running at the machine's own 34 kHz, behind its 8 kHz band limit and its gain-ranging 12-bit converters.")
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 5
                    Layout.alignment: Qt.AlignTop
                    Label {
                        text: qsTr("Program")
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    ComboBox {
                        // Reverb jargon, left in English
                        model: ["Small Hall A", "Small Hall B", "Large Hall", "Vocal Plate", "Chamber", "Percussion Plate", "Constant Density Plate", "Room"]
                        currentIndex: root.program
                        onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.reverb224ProgramKey(), index)
                        Layout.preferredWidth: 220
                    }
                }
                Switch {
                    text: qsTr("Converters")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.reverb224ConvertersKey()) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.reverb224ConvertersKey(), checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The machine's 12-bit converters: quiet signals keep their resolution, and the noise follows the signal")
                    Layout.alignment: Qt.AlignBottom
                }
                Item {
                    Layout.fillWidth: true
                }
            }

            SectionLabel {
                text: qsTr("Decay")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterKnob {
                    label: qsTr("Bass Decay")
                    suffix: "s"
                    mapping: "exponential"
                    mapMin: 0.6
                    mapMax: 70.0
                    isInteger: false
                    scale: 1000
                    key: effectRackController.reverb224BassDecayKey()
                }
                ParameterKnob {
                    label: qsTr("Crossover")
                    suffix: "Hz"
                    mapping: "logFrequency"
                    mapMin: 100
                    mapMax: 10900
                    scale: 1000
                    key: effectRackController.reverb224CrossoverKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Where Bass Decay hands over to Mid Decay")
                }
                ParameterKnob {
                    label: qsTr("Mid Decay")
                    suffix: "s"
                    mapping: "exponential"
                    mapMin: 0.6
                    mapMax: 70.0
                    isInteger: false
                    scale: 1000
                    key: effectRackController.reverb224MidDecayKey()
                }
                ParameterKnob {
                    label: qsTr("Treble Decay")
                    suffix: "Hz"
                    mapping: "logFrequency"
                    mapMin: 100
                    mapMax: 10900
                    scale: 1000
                    key: effectRackController.reverb224TrebleDecayKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Above this the tail dies away faster than the mids")
                }
            }

            SectionLabel {
                text: qsTr("Space")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterKnob {
                    id: preDelayKnob
                    label: qsTr("Pre-Delay")
                    suffix: "ms"
                    scale: 255
                    key: effectRackController.reverb224PreDelayKey()
                }
                ParameterKnob {
                    label: qsTr("Depth")
                    suffix: "%"
                    key: effectRackController.reverb224DepthKey()
                    enabled: effectRackController.reverb224HasEarlyReflections(root.program)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: enabled ? qsTr("Near, the early reflections are in front; deep, the tank takes over") : qsTr("The plates have no early reflections")
                }
                ParameterKnob {
                    label: qsTr("Diffusion")
                    suffix: "%"
                    key: effectRackController.reverb224DiffusionKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much a transient is smeared before it enters the tank")
                }
                ParameterKnob {
                    label: qsTr("Enhance")
                    suffix: "%"
                    key: effectRackController.reverb224EnhanceKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Slowly moves the tank's delays to break up its resonant modes")
                }
            }

            SectionLabel {
                text: qsTr("Output")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterKnob {
                    label: qsTr("Low Cut")
                    suffix: "Hz"
                    mapping: "logFrequencyHighPass"
                    mapMin: 20
                    mapMax: 1000
                    scale: 1000
                    key: effectRackController.reverb224LowCutKey()
                    // In the Space knobs' column
                    Layout.fillWidth: false
                    Layout.preferredWidth: preDelayKnob.width
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Keeps the lows out of the reverb, not out of the dry signal. Fully down it is off")
                }
                Item {
                    Layout.fillWidth: true
                }
            }
            // Mix and Solo together in the Space knobs' column
            RowLayout {
                spacing: 12
                Layout.fillWidth: false
                Layout.preferredWidth: preDelayKnob.width
                ParameterKnob {
                    label: qsTr("Mix")
                    suffix: "%"
                    key: effectRackController.reverb224MixKey()
                }
                Switch {
                    text: qsTr("Solo")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.reverb224SoloKey()) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.reverb224SoloKey(), checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Pass only the reverb, so it can be heard without the source")
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }
}
