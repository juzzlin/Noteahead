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
    title: "<strong>" + qsTr("Crossfeed (Slot %1)").arg(effectIndex + 1) + "</strong>"
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
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: Math.max(implicitWidth, scrollView.availableWidth)
            spacing: 16

            Label {
                text: qsTr("Gives each ear a little of the other channel, late and dark, the way a pair of speakers does. For headphones: it pulls a hard-panned part out of the middle of the head. Heard only, never rendered.")
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
                    label: qsTr("Amount")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.crossfeedAmountKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.crossfeedAmountKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much of the difference between the channels is moved to the middle")
                }

                Knob {
                    label: qsTr("Delay")
                    suffix: " " + qsTr("us")
                    mapping: "value"
                    mapMin: 100
                    mapMax: 500
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.crossfeedDelayKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.crossfeedDelayKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How much later the far ear hears it: the time sound takes to travel around a head")
                }

                Knob {
                    label: qsTr("Cutoff")
                    suffix: " Hz"
                    mapping: "logFrequency"
                    mapMin: 300
                    mapMax: 1500
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.crossfeedCutoffKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.crossfeedCutoffKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Above this the head shadows the far ear, so nothing crosses over")
                }

                Knob {
                    label: qsTr("Output")
                    suffix: " dB"
                    mapping: "value"
                    mapMin: -12
                    mapMax: 12
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.crossfeedGainKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.crossfeedGainKey(), v / 100)
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
