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
    title: "<strong>" + qsTr("Ambience (Slot %1)").arg(effectIndex + 1) + "</strong>"
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
                text: qsTr("A short, dense space to glue a mix and take the dryness off, rather than a tail to be heard. Use it on a send, or low on an insert.")
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            SectionLabel {
                text: qsTr("Space")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterKnob {
                    label: qsTr("Size")
                    suffix: "%"
                    key: effectRackController.ambienceSizeKey()
                }
                ParameterKnob {
                    label: qsTr("Pre-Delay")
                    suffix: "ms"
                    key: effectRackController.ambiencePreDelayKey()
                }
                ParameterKnob {
                    label: qsTr("Diffusion")
                    suffix: "%"
                    key: effectRackController.ambienceDiffusionKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much a transient is smeared before the space reflects it: a burst of noise rather than a slap")
                }
                ParameterKnob {
                    label: qsTr("Shape")
                    suffix: "%"
                    key: effectRackController.ambienceShapeKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The envelope of the early reflections: fading away at the bottom, even in the middle, building up at the top")
                }
                ParameterKnob {
                    label: qsTr("Early/Late")
                    suffix: "%"
                    key: effectRackController.ambienceEarlyLateKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The balance of the early reflections against the tail")
                }
            }

            SectionLabel {
                text: qsTr("Tail")
            }
            RowLayout {
                spacing: 24
                Layout.fillWidth: true
                ParameterKnob {
                    label: qsTr("Decay")
                    suffix: "s"
                    mapping: "exponential"
                    mapMin: 0.2
                    mapMax: 3.0
                    scale: 1000
                    key: effectRackController.ambienceDecayKey()
                }
                ParameterKnob {
                    label: qsTr("Damping")
                    suffix: "%"
                    key: effectRackController.ambienceDampingKey()
                }
                ParameterKnob {
                    label: qsTr("Mod Rate")
                    suffix: "Hz"
                    mapping: "exponential"
                    mapMin: 0.1
                    mapMax: 2.0
                    isInteger: false
                    scale: 1000
                    key: effectRackController.ambienceModRateKey()
                }
                ParameterKnob {
                    label: qsTr("Mod Depth")
                    suffix: "%"
                    key: effectRackController.ambienceModDepthKey()
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How far the tail drifts. A few cents at most: what keeps a short decay from ringing metallic")
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
                    key: effectRackController.ambienceLowCutKey()
                }
                ParameterKnob {
                    label: qsTr("High Cut")
                    suffix: "Hz"
                    mapping: "logFrequency"
                    mapMin: 1000
                    mapMax: 20000
                    scale: 1000
                    key: effectRackController.ambienceHighCutKey()
                }
                ParameterKnob {
                    label: qsTr("Width")
                    suffix: "%"
                    mapping: "value"
                    mapMin: 0
                    mapMax: 200
                    scale: 200
                    key: effectRackController.ambienceWidthKey()
                }
                ParameterKnob {
                    label: qsTr("Mix")
                    suffix: "%"
                    key: effectRackController.ambienceMixKey()
                }
                Switch {
                    text: qsTr("Solo")
                    checked: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.ambienceSoloKey()) > 0.5;
                    }
                    onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.ambienceSoloKey(), checked ? 1 : 0)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Pass only the space, so it can be heard without the source")
                    Layout.alignment: Qt.AlignVCenter
                }
            }
        }
    }
}
