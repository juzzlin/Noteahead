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
    title: "<strong>" + qsTr("Gate (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
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
            spacing: 16

            Label {
                text: qsTr("Pulls the quiet parts further down and lets the loud ones through: a gate for tightening drums or removing a noise floor, and a downward expander when Ratio is low. Hold and the six dB hysteresis are what keep it from chattering on a level sitting near the threshold.")
                color: "#aaa"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            EffectPresetRow {
                effectIndex: root.effectIndex
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Knob {
                    label: qsTr("Threshold")
                    suffix: " dB"
                    mapping: "value"
                    mapMin: -60
                    mapMax: 0
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.gateThresholdKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gateThresholdKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Below this the gate closes. It opens again six dB higher, so a level wobbling about it cannot rattle the gate")
                }

                Knob {
                    label: qsTr("Ratio")
                    suffix: ":1"
                    mapping: "value"
                    mapMin: 1
                    mapMax: 100
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.gateRatioKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gateRatioKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How steeply the level falls away below the threshold. Low is an expander leaning on the quiet parts, high is a gate that slams")
                }

                Knob {
                    label: qsTr("Range")
                    suffix: " dB"
                    mapping: "value"
                    mapMin: 0
                    mapMax: 90
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.gateRangeKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gateRangeKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How far down it is allowed to go at all, so a drum can be tightened rather than silenced")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Knob {
                    label: qsTr("Attack")
                    suffix: " ms"
                    mapping: "exponential"
                    mapMin: 0.05
                    mapMax: 100
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.gateAttackKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gateAttackKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How fast it opens. Slow enough and the front of every hit is swallowed")
                }

                Knob {
                    label: qsTr("Hold")
                    suffix: " ms"
                    mapping: "value"
                    mapMin: 0
                    mapMax: 500
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.gateHoldKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gateHoldKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How long it stays open after the level falls away, which is what stops it chattering")
                }

                Knob {
                    label: qsTr("Release")
                    suffix: " ms"
                    mapping: "exponential"
                    mapMin: 5
                    mapMax: 2000
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.gateReleaseKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gateReleaseKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How fast it closes once the hold has run out")
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
