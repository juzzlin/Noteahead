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

AnimatedDialog {
    id: root
    property int effectIndex: -1
    title: "<strong>" + qsTr("Stereo Enhancer Parameters (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true
    width: 640
    height: 600

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        GridLayout {
            width: scrollView.availableWidth
            columns: 2
            columnSpacing: 30
            rowSpacing: 16

            Knob {
                Layout.row: 0
                Layout.column: 0
                label: qsTr("Bass Gain")
                suffix: "%"
                from: 0
                to: 100
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerBassGainKey()) * 100;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerBassGainKey(), v / 100)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 0
                Layout.column: 1
                label: qsTr("Bass Freq")
                suffix: "Hz"
                mapping: "logFrequency"
                mapMin: 40
                mapMax: 400
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerBassFreqKey()) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerBassFreqKey(), v / 1000)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 1
                Layout.column: 0
                label: qsTr("Mid Gain")
                suffix: "%"
                from: 0
                to: 100
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerMidGainKey()) * 100;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerMidGainKey(), v / 100)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 1
                Layout.column: 1
                label: qsTr("Mid Q")
                suffix: ""
                mapping: "exponential"
                mapMin: 0.3
                mapMax: 10
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerMidQKey()) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerMidQKey(), v / 1000)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 2
                Layout.column: 0
                label: qsTr("Hi Gain")
                suffix: "%"
                from: 0
                to: 100
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerHighGainKey()) * 100;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerHighGainKey(), v / 100)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 2
                Layout.column: 1
                label: qsTr("Hi Freq")
                suffix: "Hz"
                mapping: "logFrequency"
                mapMin: 1500
                mapMax: 16000
                from: 0
                to: 1000
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerHighFreqKey()) * 1000;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerHighFreqKey(), v / 1000)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 3
                Layout.column: 0
                label: qsTr("Out Gain")
                suffix: "dB"
                from: -12
                to: 12
                value: {
                    effectRackController.revision;
                    return (effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerGainKey()) - 0.5) * 24;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerGainKey(), v / 24 + 0.5)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 3
                Layout.column: 1
                label: qsTr("Spread")
                suffix: "%"
                from: 0
                to: 100
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerSpreadKey()) * 100;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerSpreadKey(), v / 100)
                Layout.fillWidth: true
            }

            Knob {
                Layout.row: 4
                Layout.column: 0
                label: qsTr("Mix")
                suffix: "%"
                from: 0
                to: 100
                value: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerMixKey()) * 100;
                }
                onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerMixKey(), v / 100)
                Layout.fillWidth: true
            }

            CheckBox {
                Layout.row: 4
                Layout.column: 1
                text: qsTr("Solo Mode")
                checked: {
                    effectRackController.revision;
                    return effectRackController.parameterValue(root.effectIndex, effectRackController.stereoEnhancerSoloKey()) > 0.5;
                }
                onToggled: effectRackController.setParameterValue(root.effectIndex, effectRackController.stereoEnhancerSoloKey(), checked ? 1 : 0)
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Pass only what the enhancer adds, so it can be heard on its own")
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }
}
