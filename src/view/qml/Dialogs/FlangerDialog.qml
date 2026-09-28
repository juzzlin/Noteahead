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
    title: "<strong>" + qsTr("Flanger (Slot %1)").arg(effectIndex + 1) + "</strong>"
    modal: true
    focus: true

    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#333"
        radius: 2
    }

    component ComboBoxColumn: ColumnLayout {
        id: comboBoxColumn
        property string label: ""
        property alias model: comboBox.model
        property int currentIndex: 0
        signal activated(int index)
        spacing: 5
        Layout.alignment: Qt.AlignTop
        Label {
            text: comboBoxColumn.label
            font.bold: true
            font.pixelSize: 11
            color: "#aaa"
        }
        ComboBox {
            id: comboBox
            currentIndex: comboBoxColumn.currentIndex
            onActivated: index => comboBoxColumn.activated(index)
            Layout.fillWidth: true
        }
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
                text: qsTr("A swept short delay summed back with the dry signal. The fixed time offset combs at harmonically spaced notches and sweeping it drags the whole series along, which the ear follows as one moving resonance -- the jet whoosh a phaser cannot do, because its notches are not harmonically related.")
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

                ComboBoxColumn {
                    label: qsTr("Mode")
                    model: effectRackController.lfoModeNames()
                    currentIndex: {
                        effectRackController.revision;
                        return Math.round(effectRackController.parameterValue(root.effectIndex, effectRackController.flangerModeKey()));
                    }
                    onActivated: index => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerModeKey(), index)
                    Layout.fillWidth: true
                }

                Knob {
                    label: qsTr("Rate")
                    suffix: " Hz"
                    mapping: "lfoFrequency"
                    mapMin: 0.05
                    mapMax: 20
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.flangerRateKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerRateKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How fast the sweep runs. In BPM mode it reads in beat divisions instead")
                }

                Knob {
                    label: qsTr("Rate Divider")
                    mapping: "integer"
                    suffix: ""
                    from: 1
                    to: effectRackController.flangerMaxRateDivider()
                    stepSize: 1
                    value: {
                        effectRackController.revision;
                        return Math.max(1, effectRackController.parameterValue(root.effectIndex, effectRackController.flangerRateDividerKey()));
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerRateDividerKey(), Math.round(v))
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Divides the rate, so the sweep can crawl over several bars instead of over a second")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Knob {
                    label: qsTr("Delay")
                    suffix: " ms"
                    mapping: "exponential"
                    mapMin: 0.2
                    mapMax: 10
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.flangerDelayKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerDelayKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Shortest delay the sweep reaches. Past ten milliseconds the copy stops combing and is heard as a chorus")
                }

                Knob {
                    label: qsTr("Depth")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.flangerDepthKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerDepthKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How far above the Delay setting the sweep travels")
                }

                Knob {
                    label: qsTr("Feedback")
                    suffix: "%"
                    mapping: "value"
                    mapMin: -92
                    mapMax: 92
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.flangerFeedbackKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerFeedbackKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Sharpens the peaks between the notches. The sign matters as much as the amount: the two polarities comb at different frequencies")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Knob {
                    label: qsTr("Stereo Phase")
                    suffix: "°"
                    mapping: "value"
                    mapMin: 0
                    mapMax: 180
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.flangerStereoPhaseKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerStereoPhaseKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("How far apart the two channels sweep")
                }

                Knob {
                    label: qsTr("Mix")
                    suffix: "%"
                    from: 0
                    to: 100
                    value: {
                        effectRackController.revision;
                        return effectRackController.parameterValue(root.effectIndex, effectRackController.flangerMixKey()) * 100;
                    }
                    onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.flangerMixKey(), v / 100)
                    ToolTip.delay: Constants.toolTipDelay
                    ToolTip.timeout: Constants.toolTipTimeout
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("The comb is the sum of the dry and the delayed copy, so this is the depth of the notches rather than a convenience")
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
}
